#pragma once

#include "runner.hpp"

// 负责启动一个线程将Frame与result取出让drawer绘制，并将绘制结果存入outputQueue中
template<typename DrawerType, typename TrackingFrameQueueType, typename InputQueueType, typename OutputQueueType>
class Drawer: public Runner
{
public:
    Drawer(DrawerType &drawer, TrackingFrameQueueType &trackingFrameQueue, InputQueueType &inputQueue, OutputQueueType &outputQueue): 
        _drawer(drawer), 
        _trackingFrameQueue(trackingFrameQueue), 
        _inputQueue(inputQueue), 
        _outputQueue(outputQueue) {}

protected:
    void Run(){
        auto [input] = _inputQueue.Get(_inputQueueId);
        if(auto trackingResults = _trackingFrameQueue.GetNoWait(_trackingFrameQueueId); trackingResults)
            _trackingResults = std::get<0>(*trackingResults);
        _outputQueue.Put(_drawer.DrawFrame(input, _trackingResults));
    }
private:
    DrawerType &_drawer;
    typename TrackingFrameQueueType::DataID _trackingFrameQueueId;
    TrackingFrameQueueType &_trackingFrameQueue;
    typename TrackingFrameQueueType::ValueType _trackingResults;  // 缓存识别的结果，实现“跳帧”检测
    typename InputQueueType::DataID _inputQueueId;
    InputQueueType &_inputQueue;
    OutputQueueType &_outputQueue;
};