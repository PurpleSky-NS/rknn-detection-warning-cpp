#pragma once

#include <cpp-base64/base64.h>
#include "runner.hpp"

// 负责启动一个线程从帧队列里拉数据并用detector检测后放入resultQueue中
template<typename AlerterType, typename ResultFrameQueueType>
class Alerter: public Runner
{
public:
    Alerter(AlerterType &alerter, ResultFrameQueueType &resultFrameQueue): 
        _alerter(alerter), _resultFrameQueue(resultFrameQueue) {}

protected:
    void Run(){
        auto [result, frame] = _resultFrameQueue.Get(_resultFrameQueueId);
        _alerter.Update(result, frame);
    }
private:
    AlerterType &_alerter;
    typename ResultFrameQueueType::DataID _resultFrameQueueId;
    ResultFrameQueueType &_resultFrameQueue;
};