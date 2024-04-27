#pragma once

#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename AlerterType, typename TrackingFrameQueueType>
class Alerter: public Runner
{
public:
    Alerter(AlerterType &alerter, TrackingFrameQueueType &trackingFrameQueue): 
        _alerter(alerter), _trackingFrameQueue(trackingFrameQueue) {}

protected:
    void Run(){
        auto [trakingResult, frame] = _trackingFrameQueue.Get(_trackingFrameQueueId);
        _alerter.Update(trakingResult, frame);
    }
private:
    AlerterType &_alerter;
    typename TrackingFrameQueueType::DataID _trackingFrameQueueId;
    TrackingFrameQueueType &_trackingFrameQueue;
};