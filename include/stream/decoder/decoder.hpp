#pragma once
#include <type_traits>
#include "runner.hpp"

// 负责启动一个线程将pktQueue的数据解码后放入frameQueue中
template<typename DecoderType, typename PacketQueueType, typename FrameQueueType>
class Decoder: public Runner
{
public:
    Decoder(DecoderType &decoder, PacketQueueType &packetQueue, FrameQueueType &frameQueue): 
        _decoder(decoder), 
        _packetQueue(packetQueue), 
        _frameQueue(frameQueue) {}

protected:
    void Run(){
        auto frame = _decoder.Decode(std::get<0>(_packetQueue.Get(_packetQueueId)));
        if(frame)
            _frameQueue.Put(frame);
    }
private:
    DecoderType &_decoder;
    typename PacketQueueType::DataID _packetQueueId;
    PacketQueueType &_packetQueue;
    FrameQueueType &_frameQueue;
};