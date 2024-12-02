#pragma once

#include "runner.h"
#include "summary.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename AlerterType, typename TrackingFrameQueueType>
class Alerter: public Runner
{
public:
    Alerter(AlerterType &alerter, TrackingFrameQueueType &trackingFrameQueue): 
        Runner("报警器"), _alerter(alerter), _trackingFrameQueue(trackingFrameQueue) {}

protected:
    void Run(){
        if(auto [trakingResult, frame] = _trackingFrameQueue.Get(_trackingFrameQueueId); trakingResult && frame)
            _alerter(trakingResult, frame);
    }
private:
    AlerterType &_alerter;
    typename TrackingFrameQueueType::DataID _trackingFrameQueueId;
    TrackingFrameQueueType &_trackingFrameQueue;
};