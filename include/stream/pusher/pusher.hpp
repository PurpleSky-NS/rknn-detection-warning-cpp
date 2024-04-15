#pragma once

#include <future>
#include "runner.hpp"

// 负责启动一个线程将queue中的数据不断放入pusher中
template<typename PusherType, typename QueueType>
class Pusher: public Runner
{
public:
    Pusher(PusherType &pusher, QueueType &queue): _pusher(pusher), _queue(queue) {}

protected:
    void Run(){
        _pusher.Push(std::get<0>(_queue.Get(_queueId)));
    }
private:
    PusherType &_pusher;
    QueueType &_queue;
    typename QueueType::DataID _queueId;
};