#pragma once

#include <string>
#include <memory>
extern "C"
{
#include <libavformat/avformat.h>
}


/** 方便基于FFmpeg解析视频流用，这个类处理到数据包那一步 */
class PacketPuller
{
public:
    PacketPuller(const std::string &url, bool dumpFmt=false);
    PacketPuller(const PacketPuller&)=delete;
    PacketPuller(PacketPuller&&)=delete;
    ~PacketPuller();

    uint GetWidth()const;
    uint GetHeight()const;
    double GetFPS()const;

    std::shared_ptr<AVPacket> Pull();

private:
    friend class SoftDecoder;
    friend class PacketPusher;
    uint _videoStreamIndex;
    AVFormatContext *_fmtCtx=nullptr;
}; 