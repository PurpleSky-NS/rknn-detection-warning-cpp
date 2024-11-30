#pragma once

#include "runner.hpp"
#include "summary.hpp"

// 负责启动一个线程将puller中的数据不断放入queue中
template<typename PullerType, typename QueueType>
class Puller: public Runner
{
public:
    Puller(PullerType &puller, QueueType &queue): _puller(puller), _queue(queue) {}

protected:
    void Run(){
        _queue.Put(_puller());
        fpsSummary.Count("Puller");
    }
private:
    PullerType &_puller;
    QueueType &_queue;
};