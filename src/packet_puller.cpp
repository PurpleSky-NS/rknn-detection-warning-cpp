#include "stream/puller/packet.h"

#include <spdlog/spdlog.h>

PacketPuller::PacketPuller(const std::string &url, bool dumpFmt)
{
    //=========================== 创建AVFormatContext结构体 ===============================//
    _fmtCtx = avformat_alloc_context();
    //==================================== 打开文件 ======================================//
    AVDictionary *options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    if((avformat_open_input(&_fmtCtx, url.c_str(), nullptr, &options)) != 0) {
        spdlog::critical("打开视频流[{}]失败！", url);
        throw std::invalid_argument("视频流加载失败");
    }

    //=================================== 获取视频流信息 ===================================//
    if((avformat_find_stream_info(_fmtCtx, NULL)) < 0) {
        spdlog::critical("获取视频流信息失败！");
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
        spdlog::critical("获取视频流信息失败！");
        throw std::runtime_error("视频流加载失败");
    }

    //打印输入和输出信息：长度 比特率 流格式等
    if(dumpFmt)
        av_dump_format(_fmtCtx, 0, url.c_str(), 0);
}

PacketPuller::~PacketPuller()
{
    avformat_close_input(&_fmtCtx);
}

uint PacketPuller::GetWidth()const
{
    return _fmtCtx->streams[_videoStreamIndex]->codecpar->width;
}

uint PacketPuller::GetHeight()const
{
    return _fmtCtx->streams[_videoStreamIndex]->codecpar->height;
}

double PacketPuller::GetFPS()const
{
    return av_q2d(_fmtCtx->streams[_videoStreamIndex]->avg_frame_rate);
}

std::shared_ptr<AVPacket> PacketPuller::Pull()
{
    auto pkt = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *p){av_packet_unref(p);av_packet_free(&p);});
    do{
        if(av_read_frame(_fmtCtx, pkt.get()) < 0){
            spdlog::critical("读取视频流失败");
            throw std::runtime_error("读取视频流失败");
        }
    }while(static_cast<uint>(pkt->stream_index) != _videoStreamIndex);
    return pkt;
}
