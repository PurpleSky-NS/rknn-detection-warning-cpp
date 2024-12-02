#include "runner.h"
#include "summary.hpp"

Runner::~Runner(){
    Stop();
    Wait();
}

void Runner::Start(){
    if(_running) return;
    _running = true;
    _thread = std::thread(&Runner::RunThread, this);
}

void Runner::Wait(){
    if(_thread.joinable())
        _thread.join();
}

void Runner::Stop(){
    _running = false;
}

void Runner::RunThread(){
    spdlog::info("任务线程[{}]已启动", _name);
    while(_running){
        Run();
        fpsSummary.Count(_name);
    }
    spdlog::info("任务线程[{}]已退出", _name);
}