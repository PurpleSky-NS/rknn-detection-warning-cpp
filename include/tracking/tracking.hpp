#pragma once
#include "runner.hpp"
#include "summary.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename TrackingType, typename ResultFrameQueueType, typename TrackingFrameQueueType>
class Tracking: public Runner
{
public:
    Tracking(TrackingType &tracking, ResultFrameQueueType &resultFrameQueue, TrackingFrameQueueType &trackingFrameQueue): 
        _tracking(tracking), _resultFrameQueue(resultFrameQueue), _trackingFrameQueue(trackingFrameQueue) {}

protected:
    void Run(){
        auto [result, frame] = _resultFrameQueue.Get(_resultFrameQueueId);
        _trackingFrameQueue.Put(_tracking(result, frame), frame);
        fpsSummary.Count("Tracking");
    }
private:
    TrackingType &_tracking;
    typename ResultFrameQueueType::DataID _resultFrameQueueId;
    ResultFrameQueueType &_resultFrameQueue;
    TrackingFrameQueueType &_trackingFrameQueue;
};