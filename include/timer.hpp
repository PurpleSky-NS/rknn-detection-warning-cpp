#pragma once
#include <chrono>
#include <spdlog/spdlog.h>

class Timer
{
public:
    Timer(): Timer("未知操作") {}
    Timer(std::string what): _what(what){}
    ~Timer(){
        spdlog::debug("[{}]花费时间为[{}]ms", _what, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _tm).count());
    }
private:
    decltype(std::chrono::steady_clock::now()) _tm = std::chrono::steady_clock::now();
    std::string _what;
};

class FPSCounter
{
public:
    FPSCounter(): FPSCounter("未知操作") {}
    FPSCounter(std::string what): _what(what){}
    void Update(){
        _count++;
        if(auto now = std::chrono::steady_clock::now(); now - _tm > std::chrono::seconds(1)){
            spdlog::debug("[{}]FPS: {}", _what, _count);
            _count = 0;
            _tm = now;
        }
    }

private:
    size_t _count = 0;
    decltype(std::chrono::steady_clock::now()) _tm = std::chrono::steady_clock::now();
    std::string _what;
};