#pragma once

#include <thread>

// 负责启动一个线程将puller中的数据不断放入squeue中
class Runner
{
public:
    Runner(): _running(true), _thread(&Runner::_RunThread, this) {}
    virtual ~Runner(){
        Stop();
    }

    void Wait(){
        if(_running && _thread.joinable())
            _thread.join();
    }

    void Stop(){
        _running = false;
    }

protected:
    virtual void _Run() = 0;
private:
    bool _running;
    std::thread _thread;

    void _RunThread(){
        while(_running){
            _Run();
        }
    }
};