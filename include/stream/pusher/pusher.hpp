#pragma once

#include <future>
#include "runner.hpp"

// 负责启动一个线程将queue中的数据不断放入pusher中
template<typename PusherType, typename QueueType>
class Pusher: public Runner
{
public:
    Pusher(PusherType &pusher, QueueType &queue): _pusher(pusher), _queue(queue), _queueId(QueueType::DEFAULT_ID_VALUE), _sleepDuration(std::chrono::milliseconds(static_cast<int>(1000. / pusher.GetFPS()))) {}

protected:
    void _Run(){
        auto f = std::async([&](){_pusher.PushFrame(_queue.Get(_queueId));});
        std::this_thread::sleep_for(_sleepDuration);
    }
private:
    PusherType &_pusher;
    QueueType &_queue;
    typename QueueType::IdType _queueId;
    std::chrono::milliseconds _sleepDuration;
};