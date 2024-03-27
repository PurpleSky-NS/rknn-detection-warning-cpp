#pragma once
#include <iostream>
#include <chrono>

template <typename To>
class Timer{
public:
    Timer(std::string what): tm(std::chrono::system_clock::now()), what(what){}
    ~Timer(){
        std::cout << what << std::chrono::duration_cast<To>(std::chrono::system_clock::now() - tm).count() << std::endl;
    }
private:
    decltype(std::chrono::system_clock::now()) tm;
    std::string what;
};