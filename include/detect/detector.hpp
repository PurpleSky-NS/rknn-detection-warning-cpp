#pragma once

#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename DetectorType, typename FrameQueueType, typename ResultQueueType>
class Detector: public Runner
{
public:
    Detector(DetectorType &detector, const FrameQueueType &frameQueue, ResultQueueType &resultFrameQueue): 
        _detector(detector), _frameQueue(frameQueue), _resultFrameQueue(resultFrameQueue) {}

protected:
    void Run(){
        auto [frame] = _frameQueue.Get(_frameQueueId);
        auto result = _detector.Detect(frame);
        _resultFrameQueue.Put(result, frame);
    }
private:
    DetectorType &_detector;
    typename FrameQueueType::DataID _frameQueueId;
    const FrameQueueType &_frameQueue;
    ResultQueueType &_resultFrameQueue;
};