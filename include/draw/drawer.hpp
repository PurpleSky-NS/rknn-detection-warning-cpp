#pragma once

#include "runner.hpp"

// 负责启动一个线程将puller中的数据不断放入squeue中
template<typename DrawerType, typename ResultQueueType, typename InputQueueType, typename OutputQueueType>
class Drawer: public Runner
{
public:
    Drawer(DrawerType &drawer, const ResultQueueType &resultQueue, const InputQueueType &inputQueue, OutputQueueType &outputQueue): 
        _drawer(drawer), 
        _resultQueue(resultQueue), 
        _inputQueue(inputQueue), 
        _outputQueue(outputQueue) {}

protected:
    void Run(){
        auto [input] = _inputQueue.Get(_inputQueueId);
        if(auto results = _resultQueue.GetNoWait(_resultQueueId); results)
            _results = std::get<0>(*results);
        _outputQueue.Put(_drawer.DrawFrame(input, _results));
    }
private:
    DrawerType &_drawer;
    typename ResultQueueType::DataID _resultQueueId;
    const ResultQueueType &_resultQueue;
    typename ResultQueueType::ValueType _results;  // 缓存识别的结果，实现“跳帧”检测
    typename InputQueueType::DataID _inputQueueId;
    const InputQueueType &_inputQueue;
    OutputQueueType &_outputQueue;
};