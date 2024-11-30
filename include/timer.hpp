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
