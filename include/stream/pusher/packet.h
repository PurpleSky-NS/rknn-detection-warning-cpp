#pragma once

#include "stream/puller/packet.h"

class PacketPusher
{
public:
    PacketPusher(const PacketPuller &pktPuller, const std::string &output);
    PacketPusher(const PacketPusher&)=delete;
    PacketPusher(PacketPusher&&)=delete;
    ~PacketPusher();
    
    //有下面两种方式推送帧
    void Push(std::shared_ptr<AVPacket> pkt);
    PacketPusher &operator<<(AVPacket &pkt);

private:
    AVFormatContext *_inputFmtCtx=nullptr;
    AVFormatContext *_outputFmtCtx=nullptr;
    int64_t _lastDts = 0;
};
