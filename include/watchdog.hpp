#pragma once

#include <functional>
#include "runner.hpp"

class WatchDog: public Runner {
public:
    /* 
        可以设置特定的超时时间，超时后触发回调，注意可能2*超时时间后才会触发异常
        比如在第一轮休眠的开始正常喂狗了，但紧接着模块就不正常了，那么第一轮休眠后仍不会触发异常，等到第二轮休眠苏醒后才会触发异常
        其中休眠了2*超时时间，因此最多2*超时时间后会触发异常
    */
    WatchDog(uint timeoutSeconds, std::function<void()> timeoutCallback): _timeoutSeconds(timeoutSeconds), _timeoutCallback(timeoutCallback) {};

    /* 喂一次狗，下次苏醒检查时不触发超时 */
    inline void Feed();

private:
    uint _timeoutSeconds;
    std::function<void()> _timeoutCallback;
    bool _food = false;

    void Run(){
        std::this_thread::sleep_for(std::chrono::seconds(_timeoutSeconds));
        if (!_food) 
            _timeoutCallback();
        _food = false;
    }
};

void WatchDog::Feed() {
    _food = true;
}