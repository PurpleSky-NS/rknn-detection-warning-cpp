#pragma once

#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename DetectorType, typename FrameQueueType, typename ResultQueueType>
class Detector: public Runner
{
public:
    Detector(DetectorType &detector, const FrameQueueType &frameQueue, ResultQueueType &resultQueue): 
        _detector(detector), _frameQueue(frameQueue), _resultQueue(resultQueue) {}

protected:
    void _Run(){
        _resultQueue.Put(_detector.Detect(std::get<0>(_frameQueue.Get(_frameQueueId))));
    }
private:
    DetectorType &_detector;
    typename FrameQueueType::DataID _frameQueueId;
    const FrameQueueType &_frameQueue;
    ResultQueueType &_resultQueue;
};