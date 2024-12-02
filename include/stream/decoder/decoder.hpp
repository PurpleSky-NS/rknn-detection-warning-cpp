#pragma once
#include <type_traits>
#include "runner.h"
#include "summary.hpp"

// 负责启动一个线程将pktQueue的数据解码后放入frameQueue中
template<typename DecoderType, typename PacketQueueType, typename FrameQueueType>
class Decoder: public Runner
{
public:
    Decoder(DecoderType &decoder, PacketQueueType &packetQueue, FrameQueueType &frameQueue): 
        Runner("解码器"),
        _decoder(decoder), 
        _packetQueue(packetQueue), 
        _frameQueue(frameQueue) {}

protected:
    void Run(){
        if(auto [pkt] = _packetQueue.Get(_packetQueueId); pkt)
            if(auto frame = _decoder(pkt);frame)
                _frameQueue.Put(frame);
    }
private:
    DecoderType &_decoder;
    typename PacketQueueType::DataID _packetQueueId;
    PacketQueueType &_packetQueue;
    FrameQueueType &_frameQueue;
};