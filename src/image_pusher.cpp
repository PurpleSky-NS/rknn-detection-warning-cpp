#include "stream/pusher/image.h"
#include <spdlog/spdlog.h>

ImagePusher::ImagePusher(const std::string &output, const cv::Size &size, double fps, uint pusherCBR, uint thread, bool h265, bool lowLatency)
{
    char errBuf[1024];
    if (auto ret = avformat_alloc_output_context2(&_outputFmtCtx, nullptr, "flv", output.c_str()); ret < 0) {
        spdlog::critical("输出上下文构造失败");
        throw std::runtime_error("推流失败");
    }

    // 2. 查找视频编码器
    const AVCodec* codec = avcodec_find_encoder(h265 ? AV_CODEC_ID_H265 : AV_CODEC_ID_H264);
    spdlog::info("视频流编码器为{}", h265 ? "H265" : "H264");
    if (!codec) {
        spdlog::critical("编码器未找到");
        throw std::runtime_error("推流失败");
    }

    // 3. 创建编码器上下文
    _codecCtx = avcodec_alloc_context3(codec);
    if (!_codecCtx) {
        spdlog::critical("编码器上下文创建失败");
        throw std::runtime_error("推流失败");
    }

    if(lowLatency){
        av_opt_set(_codecCtx->priv_data, "tune", "zerolatency", 0);
        spdlog::info("已设为低延迟模式");
    }
    _codecCtx->bit_rate = pusherCBR * 1024 * 8;  // 转为KB
    _codecCtx->width = size.width;  // 设置分辨率
    _codecCtx->height = size.height;
    _codecCtx->thread_count = thread;
    _codecCtx->time_base = av_d2q(1 / fps, 1000);  // 25帧每秒
    _codecCtx->framerate = av_d2q(fps, 1000);  // 25帧每秒
    _codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    _codecCtx->gop_size = 2;
    _codecCtx->max_b_frames = 0;
    _codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 打开编码器
    if (auto ret = avcodec_open2(_codecCtx, codec, nullptr); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("编码器打开失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    // 4. 创建流并将其添加到格式上下文
    _videoStream = avformat_new_stream(_outputFmtCtx, codec);
    if (!_videoStream) {
        spdlog::critical("输出流构造失败");
        throw std::runtime_error("推流失败");
    }
    _videoStream->codecpar->codec_tag = 0;

    // 从编码器复制参数
    if (auto ret = avcodec_parameters_from_context(_videoStream->codecpar, _codecCtx); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流信息复制失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    // 5. 打开输出RTMP流
    if (auto ret = avio_open(&_outputFmtCtx->pb, output.c_str(), AVIO_FLAG_WRITE); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流打开失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    // 6. 写入流头
    if (auto ret = avformat_write_header(_outputFmtCtx, nullptr); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流头信息写入失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }
    
    // 转换到 YUV420P 格式
    _swsCtx = sws_getContext(_codecCtx->width, _codecCtx->height, AV_PIX_FMT_BGR24,
                            _codecCtx->width, _codecCtx->height, AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);
                                         
    _frameYUV = av_frame_alloc();
    _frameYUV->format = _codecCtx->pix_fmt;
    _frameYUV->width = _codecCtx->width;
    _frameYUV->height = _codecCtx->height;
    if (auto ret = av_frame_get_buffer(_frameYUV, 0); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("分配帧内存失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    //打印输出信息
    av_dump_format(_outputFmtCtx, 0, output.c_str(), 1);
}

ImagePusher::~ImagePusher()
{
    av_write_trailer(_outputFmtCtx);
    avio_close(_outputFmtCtx->pb);
    av_frame_free(&_frameYUV);
    avcodec_free_context(&_codecCtx);
    avformat_free_context(_outputFmtCtx);
    sws_freeContext(_swsCtx);
}

void ImagePusher::operator()(std::shared_ptr<cv::Mat> frame)
{
    static uint8_t *indata[AV_NUM_DATA_POINTERS] = {};
    static int insize[AV_NUM_DATA_POINTERS] = {};
    static AVPacket pkt = {};
    static char errBuf[1024];

    indata[0] = frame->data;
    insize[0] = frame->cols * frame->elemSize();
    // 将图像转换为 YUV 格式
    sws_scale(_swsCtx, indata, insize, 0, frame->rows,
              _frameYUV->data, _frameYUV->linesize);

    _frameYUV->pts = _vpts++;
    // 8. 编码并发送帧
    if (auto ret = avcodec_send_frame(_codecCtx, _frameYUV); ret < 0) {
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("图像帧编码失败：{}", errBuf);
        throw std::runtime_error("推流失败");
    }

    if (avcodec_receive_packet(_codecCtx, &pkt) == 0) {
        // 将编码后的包发送到RTMP流
        pkt.pts = av_rescale_q(pkt.pts, _codecCtx->time_base, _videoStream->time_base);
        pkt.dts = av_rescale_q(pkt.dts, _codecCtx->time_base, _videoStream->time_base);
        pkt.duration = av_rescale_q(pkt.duration, _codecCtx->time_base, _videoStream->time_base);
        if (auto ret = av_interleaved_write_frame(_outputFmtCtx, &pkt); ret < 0) {
            av_strerror(ret, errBuf, sizeof(errBuf));
            spdlog::critical("发送视频帧失败：{}", errBuf);
            throw std::runtime_error("推流失败");
        }
    }
}
