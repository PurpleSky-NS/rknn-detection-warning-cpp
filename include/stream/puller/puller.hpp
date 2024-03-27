#pragma once

#include "runner.hpp"

// 负责启动一个线程将puller中的数据不断放入squeue中
template<typename PullerType, typename QueueType>
class Puller: public Runner
{
public:
    Puller(PullerType &puller, QueueType &queue): _puller(puller), _queue(queue) {}

protected:
    void _Run(){
        _queue.Put(_puller.GetFrame());
    }
private:
    PullerType &_puller;
    QueueType &_queue;
};