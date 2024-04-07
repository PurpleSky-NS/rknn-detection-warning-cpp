#pragma once

#include <thread>

// 经踩坑发现不能再构造函数中启动线程，否则会使子线程在子类没有构造完全时，调用到纯虚函数
// 负责启动一个线程，在Run中执行任务
class Runner
{
public:
    virtual ~Runner(){
        Stop();
    }

    void Start(){
        if(_running) return;
        _running = true;
        _thread = std::thread(&Runner::RunThread, this);
    }

    void Wait(){
        if(_running && _thread.joinable())
            _thread.join();
    }

    void Stop(){
        _running = false;
    }

protected:
    virtual void Run() = 0;
private:
    bool _running = false;
    std::thread _thread;

    void RunThread(){
        while(_running){
            Run();
        }
    }
};