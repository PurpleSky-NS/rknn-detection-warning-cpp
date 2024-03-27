#pragma once

#include "runner.hpp"

// 负责启动一个线程将puller中的数据不断放入squeue中
template<typename DrawerType, typename ResultQueueType, typename InputQueueType, typename OutputQueueType>
class Drawer: public Runner
{
public:
    Drawer(DrawerType &drawer, const ResultQueueType &resultQueue, const InputQueueType &inputQueue, OutputQueueType &outputQueue): 
        _drawer(drawer), 
        _resultQueueId(ResultQueueType::DEFAULT_ID_VALUE), _resultQueue(resultQueue), 
        _inputQueueId(InputQueueType::DEFAULT_ID_VALUE), _inputQueue(inputQueue), 
        _outputQueue(outputQueue) {}

protected:
    void _Run(){
        _input = std::move(_inputQueue.Get(_inputQueueId).clone());
        auto results = _resultQueue.GetNoWait(_resultQueueId);
        if(results)
            _results = std::move(*results);
        _drawer.DrawFrame(_input, _results);
        _outputQueue.Put(_input);
    }

private:
    DrawerType &_drawer;
    typename ResultQueueType::IdType _resultQueueId;
    typename ResultQueueType::DataType _results;
    const ResultQueueType &_resultQueue;
    typename InputQueueType::IdType _inputQueueId;
    typename InputQueueType::DataType _input;
    const InputQueueType &_inputQueue;
    OutputQueueType &_outputQueue;
    
};