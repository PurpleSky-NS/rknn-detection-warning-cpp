#pragma once

#include <future>
#include "runner.h"
#include "summary.hpp"

// 负责启动一个线程将queue中的数据不断放入pusher中
template<typename PusherType, typename QueueType>
class Pusher: public Runner
{
public:
    Pusher(PusherType &pusher, QueueType &queue): Runner("推流器"), _pusher(pusher), _queue(queue) {}

protected:
    void Run(){
        if(auto [img] = _queue.Get(_queueId); img)
            _pusher(img);
    }
private:
    PusherType &_pusher;
    QueueType &_queue;
    typename QueueType::DataID _queueId;
};