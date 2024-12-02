#pragma once

#include <unordered_map>
#include <spdlog/spdlog.h>
#include "runner.h"

// 模块信息统计器
class FPSSummary: public SleepRunner
{
public:
    FPSSummary(): SleepRunner("FPS统计器") {}

    /* 更新某模块FPS */
    void Count(const std::string& module);

private:
    inline static const uint _interval = 5;
    std::unordered_map<std::string, uint> _fpsSummary;
    std::mutex _mutex;

    void Run();
};
inline FPSSummary fpsSummary;

inline void FPSSummary::Count(const std::string& module)
{
    std::lock_guard<std::mutex> lock(_mutex);
    ++_fpsSummary[module];
}

inline void FPSSummary::Run()
{
    if(!Sleep(std::chrono::seconds(_interval)))
        return;
    std::lock_guard<std::mutex> lock(_mutex);
    std::string summary;
    for (auto& [module, count]: _fpsSummary){
        if (!summary.empty())
            summary += " ";
        summary += module + ":" + fmt::format("{:.2f}", (float)count / _interval);
        count = 0;
    }
    spdlog::info("各模块FPS统计信息[{}s平均]: [{}]", _interval, summary);
}
