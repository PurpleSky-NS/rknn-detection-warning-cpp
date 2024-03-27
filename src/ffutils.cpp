#include <iostream>
#include "stream/ffutils.h"

FFmpegStreamer::FFmpegStreamer(const std::string &url, bool dumpFmt)
{
    //=========================== 创建AVFormatContext结构体 ===============================//
    _fmtCtx = avformat_alloc_context();
    //==================================== 打开文件 ======================================//
    AVDictionary *options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    if((avformat_open_input(&_fmtCtx, url.c_str(), nullptr, &options)) != 0) {
        std::cerr << "打开视频流[" << url << "]失败！" << std::endl;
        throw std::invalid_argument("视频流加载失败");
    }

    //=================================== 获取视频流信息 ===================================//
    if((avformat_find_stream_info(_fmtCtx, NULL)) < 0) {
        std::cerr << "获取视频流信息失败！" << std::endl;
        throw std::runtime_error("视频流加载失败");
    }

    //分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
    //循环查找视频中包含的流信息，直到找到视频类型的流
    //便将其记录下来 保存到videoStreamIndex变量中
    for (size_t i = 0; i < _fmtCtx->nb_streams; i++) {
        if(_fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            _videoStreamIndex = i;
            break;//找到视频流就退出
        }
    }
    //如果videoStream为-1 说明没有找到视频流
    if(_videoStreamIndex == _fmtCtx->nb_streams) {
        std::cerr << "获取视频流信息失败！" << std::endl;
        throw std::runtime_error("视频流加载失败");
    }

    _pkt = av_packet_alloc();
    if(!_pkt){
        std::cerr << "视频流帧空间分配失败！" << std::endl;
        throw std::runtime_error("视频流加载失败");
    }

    //打印输入和输出信息：长度 比特率 流格式等
    if(dumpFmt)
        av_dump_format(_fmtCtx, 0, url.c_str(), 0);
}

FFmpegStreamer::~FFmpegStreamer()
{
    av_packet_free(&_pkt);
    avformat_close_input(&_fmtCtx);
}

uint FFmpegStreamer::GetWidth()const
{
    return _fmtCtx->streams[_videoStreamIndex]->codecpar->width;
}

uint FFmpegStreamer::GetHeight()const
{
    return _fmtCtx->streams[_videoStreamIndex]->codecpar->height;
}

double FFmpegStreamer::GetFPS()const
{
    return av_q2d(_fmtCtx->streams[_videoStreamIndex]->avg_frame_rate);
}

const AVPacket *FFmpegStreamer::Parse()
{
    while(av_read_frame(_fmtCtx, _pkt) >= 0)  //读取的是一帧视频  数据存入一个AVPacket的结构中
        if(static_cast<uint>(_pkt->stream_index) == _videoStreamIndex)
            return _pkt;
    return nullptr;  //失败直接返回空指针
}

FFmpegDecoder::FFmpegDecoder(const FFmpegStreamer &streamer, AVPixelFormat decodeFmt)
{
    //=================================  查找解码器 ===================================//
    auto avCodecPara = streamer._fmtCtx->streams[streamer._videoStreamIndex]->codecpar;
    auto codec = avcodec_find_decoder(avCodecPara->codec_id);
    if(!codec) {
        std::cerr << "获取解码器失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }
    //根据解码器参数来创建解码器内容
    _codecCtx = avcodec_alloc_context3(codec);
    if(!_codecCtx) {
        std::cerr << "解码器上下文构造失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }
    avcodec_parameters_to_context(_codecCtx, avCodecPara);

    //================================  打开解码器 ===================================//
    if(avcodec_open2(_codecCtx, codec, NULL) < 0) { // 具体采用什么解码器ffmpeg经过封装 我们无须知道
        std::cerr << "解码器打开失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }

    //==================================== 分配空间 ==================================//
    //一帧图像数据大小
    auto numBytes = av_image_get_buffer_size(decodeFmt, _codecCtx->width, _codecCtx->height, 1);
    _imageBuffer = (uint8_t *)av_malloc(numBytes);
    if(!_imageBuffer) {
        std::cerr << "目标图像缓存分配失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }
    
    _originalFrame = av_frame_alloc();
    if(!_originalFrame) {
        std::cerr << "原始帧结构构造分配失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }
    _decodedFrame = av_frame_alloc();
    if(!_decodedFrame) {
        std::cerr << "解码帧结构构造分配失败！" << std::endl;
        throw std::runtime_error("解码器加载失败");
    }
    av_image_fill_arrays(_decodedFrame->data, _decodedFrame->linesize, _imageBuffer, decodeFmt,
                            _codecCtx->width, _codecCtx->height, 1);
    _swsCtx = sws_getContext(
        _codecCtx->width, _codecCtx->height, _codecCtx->pix_fmt,    //源地址长宽以及数据格式
        _codecCtx->width, _codecCtx->height, decodeFmt,             //目的地址长宽以及数据格式
        SWS_BICUBIC, NULL, NULL, NULL                               //算法类型  AV_PIX_FMT_YUVJ420P   AV_PIX_FMT_BGR24
    );                    
}

FFmpegDecoder::~FFmpegDecoder()
{
    avcodec_free_context(&_codecCtx);
    av_frame_free(&_originalFrame);
    av_frame_free(&_decodedFrame);
    av_free(_imageBuffer);
}

uint8_t *FFmpegDecoder::Decode(const AVPacket *pkt)
{
    if(avcodec_send_packet(_codecCtx, pkt) == 0){
        if(avcodec_receive_frame(_codecCtx, _originalFrame) == 0){
            sws_scale(
                _swsCtx,
                _originalFrame->data,
                _originalFrame->linesize,
                0,
                _codecCtx->height,
                _decodedFrame->data,
                _decodedFrame->linesize
            );
            return _imageBuffer;
        }
    }
    return nullptr;
}
