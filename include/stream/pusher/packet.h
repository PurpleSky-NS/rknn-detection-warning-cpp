#pragma once

#include "stream/puller/packet.h"

class PacketPusher
{
public:
    // usb视频流格式是未压缩的rawvideo格式，与flv容器不兼容，因此不能直接推数据帧
    PacketPusher(const PacketPuller &pktPuller, const std::string &output);
    PacketPusher(const PacketPusher&)=delete;
    PacketPusher(PacketPusher&&)=delete;
    ~PacketPusher();
    
    void operator()(std::shared_ptr<AVPacket> pkt);

private:
    AVFormatContext *_inputFmtCtx=nullptr;
    AVFormatContext *_outputFmtCtx=nullptr;
    int64_t _lastDts = 0;
};
