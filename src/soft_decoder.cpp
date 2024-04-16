#include "stream/decoder/soft.h"

#include <spdlog/spdlog.h>

SoftDecoder::SoftDecoder(const PacketPuller &streamer)
{
    //=================================  查找解码器 ===================================//
    auto avCodecPara = streamer._fmtCtx->streams[streamer._videoStreamIndex]->codecpar;
    auto codec = avcodec_find_decoder(avCodecPara->codec_id);
    if(!codec) {
        spdlog::critical("获取解码器失败！");
        throw std::runtime_error("解码器加载失败");
    }
    //根据解码器参数来创建解码器内容
    _codecCtx = avcodec_alloc_context3(codec);
    if(!_codecCtx) {
        spdlog::critical("解码器上下文构造失败！");
        throw std::runtime_error("解码器加载失败");
    }
    avcodec_parameters_to_context(_codecCtx, avCodecPara);

    //================================  打开解码器 ===================================//
    if(avcodec_open2(_codecCtx, codec, NULL) < 0) { // 具体采用什么解码器ffmpeg经过封装 我们无须知道
        spdlog::critical("解码器打开失败！");
        throw std::runtime_error("解码器加载失败");
    }

    //==================================== 分配空间 ==================================//
    //一帧图像数据大小
    auto numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, _codecCtx->width, _codecCtx->height, 1);
    _imageBuffer = (uint8_t *)av_malloc(numBytes);
    if(!_imageBuffer) {
        spdlog::critical("图像缓存分配失败！");
        throw std::runtime_error("解码器加载失败");
    }
    
    _originalFrame = av_frame_alloc();
    if(!_originalFrame) {
        spdlog::critical("原始帧结构构造分配失败！");
        throw std::runtime_error("解码器加载失败");
    }
    _decodedFrame = av_frame_alloc();
    if(!_decodedFrame) {
        spdlog::critical("解码帧结构构造分配失败！");
        throw std::runtime_error("解码器加载失败");
    }
    av_image_fill_arrays(_decodedFrame->data, _decodedFrame->linesize, _imageBuffer, AV_PIX_FMT_BGR24,
                            _codecCtx->width, _codecCtx->height, 1);
    _swsCtx = sws_getContext(
        _codecCtx->width, _codecCtx->height, _codecCtx->pix_fmt,    //源地址长宽以及数据格式
        _codecCtx->width, _codecCtx->height, AV_PIX_FMT_BGR24,             //目的地址长宽以及数据格式
        SWS_BICUBIC, NULL, NULL, NULL                               //算法类型  AV_PIX_FMT_YUVJ420P   AV_PIX_FMT_BGR24
    );                    
}

SoftDecoder::~SoftDecoder()
{
    avcodec_free_context(&_codecCtx);
    av_frame_free(&_originalFrame);
    av_frame_free(&_decodedFrame);
    av_free(_imageBuffer);
}

std::shared_ptr<cv::Mat> SoftDecoder::Decode(std::shared_ptr<AVPacket> pkt)
{
    if(avcodec_send_packet(_codecCtx, pkt.get()) == 0){
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
            return std::make_shared<cv::Mat>(cv::Mat(_codecCtx->height, _codecCtx->width, CV_8UC3, _imageBuffer).clone());
        }
    }
    spdlog::error("解码失败！");
    return std::shared_ptr<cv::Mat>();
}
