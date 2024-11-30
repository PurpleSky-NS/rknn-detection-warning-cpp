#pragma once

#include <string>
#include <memory>
#include <opencv2/opencv.hpp>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}
#include "watchdog.hpp"

/** 方便基于FFmpeg解析视频流用，这个类处理到数据包那一步 */
class PacketPuller
{
public:
    PacketPuller(std::string source);
    PacketPuller(const PacketPuller&)=delete;
    PacketPuller(PacketPuller&&)=delete;
    ~PacketPuller();

    inline cv::Size GetSize()const;
    inline double GetFPS()const;

    std::shared_ptr<AVPacket> operator()();

private:
    friend class PacketDecoder;
    friend class PacketPusher;
    uint _videoStreamIndex;
    AVFormatContext *_fmtCtx=nullptr;
    WatchDog _timeoutMonitor;
}; 

cv::Size PacketPuller::GetSize()const
{
    return {_fmtCtx->streams[_videoStreamIndex]->codecpar->width, _fmtCtx->streams[_videoStreamIndex]->codecpar->height};
}

double PacketPuller::GetFPS()const
{
    return av_q2d(_fmtCtx->streams[_videoStreamIndex]->avg_frame_rate);
}