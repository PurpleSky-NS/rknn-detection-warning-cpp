#include "stream/puller/packet.h"
#include <chrono>
#include <spdlog/spdlog.h>
#include "summary.hpp"

PacketPuller::PacketPuller(std::string source): _timeoutMonitor("数据包拉流监控器", 60, [](){
    spdlog::critical("拉取视频流超时，视频帧拉取失败");
    throw std::runtime_error("视频帧拉取失败");
})
{
    char errBuf[1024];
    if(std::all_of(source.begin(), source.end(), [](const auto &ch){return isdigit(ch);})){
        avdevice_register_all();
        source = "/dev/video" + source;
    }
    //=========================== 创建AVFormatContext结构体 ===============================//
    _fmtCtx = avformat_alloc_context();
    //==================================== 打开文件 ======================================//
    AVDictionary *options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);

    if(auto ret = avformat_open_input(&_fmtCtx, source.c_str(), nullptr, &options); ret < 0) {
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("打开视频流[{}]失败: {}", source, errBuf);
        throw std::invalid_argument("视频流加载失败");
    }

    //=================================== 获取视频流信息 ===================================//
    if(auto ret = avformat_find_stream_info(_fmtCtx, NULL); ret < 0) {
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("获取视频流信息失败: {}", errBuf);
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
    av_dump_format(_fmtCtx, _videoStreamIndex, source.c_str(), 0);

    _timeoutMonitor.Start();
}

PacketPuller::~PacketPuller()
{
    _timeoutMonitor.Stop();
    avformat_close_input(&_fmtCtx);
}

std::shared_ptr<AVPacket> PacketPuller::operator()()
{
    auto pkt = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *p){av_packet_unref(p);av_packet_free(&p);});
    do{
        if(av_read_frame(_fmtCtx, pkt.get()) < 0){
            spdlog::critical("读取视频流失败");
            throw std::runtime_error("读取视频流失败");
        }
        _timeoutMonitor.Feed();  // 喂狗，当卡在av_read_frame时，可以触发异常使程序退出，interrupt_callback机制也可以实现，不过需要一秒会被调用300多次，在该函数里写判断开销比较大
    }while(static_cast<uint>(pkt->stream_index) != _videoStreamIndex);
    return pkt;
}
