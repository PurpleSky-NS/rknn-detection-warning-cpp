#pragma once

#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename DetectorType, typename FrameQueueType, typename ResultQueueType>
class Detector: public Runner
{
public:
    Detector(DetectorType &detector, const FrameQueueType &frameQueue, ResultQueueType &resultQueue): 
        _detector(detector), _frameQueueId(FrameQueueType::DEFAULT_ID_VALUE), _frameQueue(frameQueue), _resultQueue(resultQueue) {}

protected:
    void _Run(){
        auto r = _detector.Detect(_frameQueue.Get(_frameQueueId));
        _resultQueue.Put(r);
    }
private:
    DetectorType &_detector;
    typename FrameQueueType::IdType _frameQueueId = 0;
    const FrameQueueType &_frameQueue;
    ResultQueueType &_resultQueue;
};