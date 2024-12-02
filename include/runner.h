#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string_view>

// 经踩坑发现不能再构造函数中启动线程，否则会使子线程在子类没有构造完全时，调用到纯虚函数
// 负责启动一个线程，在Run中执行任务
class Runner
{
public:
    Runner(std::string_view name) : _name(name){}

    virtual ~Runner();

    /* 启动任务线程 */
    void Start();

    /* 等待任务线程 */
    void Wait();

    /* 停止任务线程 */
    void Stop();

protected:
    bool _running = false;
    virtual void Run() = 0;
private:
    std::string _name;
    std::thread _thread;

    void RunThread();
};

/* 自带一个Sleep方法，可以使用该方法进行线程休眠，并且可以被Stop打断退出 */
class SleepRunner: public Runner
{
public:
    SleepRunner(std::string_view name) : Runner(name){}

    inline virtual ~SleepRunner();

    /* 停止任务线程 */
    inline void Stop();

protected:
    std::mutex _mutex;
    std::condition_variable _sleeper;

    template<typename Time>
    inline bool Sleep(const Time &time);
};

SleepRunner::~SleepRunner()
{
    Stop();
}

void SleepRunner::Stop()
{
    Runner::Stop();
    _sleeper.notify_all();
}

template<typename Time>
bool SleepRunner::Sleep(const Time &time)
{
    std::unique_lock lock(_mutex);
    _sleeper.wait_for(lock, time);
    return _running;
}