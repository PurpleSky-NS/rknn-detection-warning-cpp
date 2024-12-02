#pragma once

#include <thread>
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