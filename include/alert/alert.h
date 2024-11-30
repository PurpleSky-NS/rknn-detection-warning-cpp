// 结合追踪和区域实现多区域追踪
#pragma once
#include "alert/region.h"

class TriggerAlerter
{
public:
    TriggerAlerter() = delete;
    TriggerAlerter(
        const std::vector<std::string> &regionsInfo, const cv::Size &size, 
        const std::vector<std::string> &alertsInfo, const std::vector<std::string> &classes
    );
    TriggerAlerter(const TriggerAlerter &)=delete;
    TriggerAlerter(TriggerAlerter &&)=delete;

    // 更新所有区域
    void operator()(std::shared_ptr<TrackerWorld> trackers, std::shared_ptr<cv::Mat> image);

    // 获取所有追踪类
    const std::unordered_set<size_t> &GetAlertClasses()const;

    // 获取所有追踪类
    const std::vector<Region> &GetRegions()const;

private:
    std::vector<Region> _regions;
    std::unordered_set<size_t> _trackClasses;
};