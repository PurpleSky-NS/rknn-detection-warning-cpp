#pragma once

#include <random>
#include <memory>
#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入ResultQueueType中
template<typename DetectorType, typename FrameQueueType, typename ResultQueueType>
class Detector: public Runner
{
public:
    Detector(DetectorType &detector, FrameQueueType &frameQueue, ResultQueueType &resultFrameQueue): 
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
    FrameQueueType &_frameQueue;
    ResultQueueType &_resultFrameQueue;
};

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename DetectorType, typename FrameQueueType, typename ResultQueueType>
class DummyDetector: public Runner
{
public:
    DummyDetector(DetectorType &detector, FrameQueueType &frameQueue, ResultQueueType &resultFrameQueue): 
        _detector(detector), _frameQueue(frameQueue), _resultFrameQueue(resultFrameQueue), _rdPos(-20, 20), _rdSize(-10, 20), _rdB(.6) {}

protected:
    void Run(){
        _obj.box.x += _rdPos(_e); 
        _obj.box.y += _rdPos(_e);
        int w = static_cast<int>(_obj.box.w) + _rdSize(_e); 
        int h = static_cast<int>(_obj.box.h) + _rdSize(_e); 
        _obj.box.w = w >= 0 ? w : 0;
        _obj.box.h = h >= 0 ? h : 0;
        auto [frame] = _frameQueue.Get(_frameQueueId);
        
        auto res = std::make_shared<typename ResultQueueType::DataType>(_rdB(_e) ? typename ResultQueueType::DataType{_obj} : typename ResultQueueType::DataType());
        _resultFrameQueue.Put(res, frame);
    }
private:
    DetectorType &_detector;
    typename FrameQueueType::DataID _frameQueueId;
    FrameQueueType &_frameQueue;
    ResultQueueType &_resultFrameQueue;

    size_t _dummyFrameCount = 0;
    Object _obj{50, 50, 75, 75, 0.5, 0, "person"};
    std::default_random_engine _e;
    std::uniform_int_distribution<int> _rdPos;
    std::uniform_int_distribution<int> _rdSize;
    std::bernoulli_distribution _rdB;
};