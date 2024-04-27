// 结合追踪和区域实现多区域追踪
#pragma once
#include "alert/region.h"

class TriggerAlerter
{
public:
    TriggerAlerter() = delete;
    TriggerAlerter(
        const std::vector<std::string> &regionsInfo, size_t w, size_t h, 
        const std::vector<std::string> &alertsInfo, const std::vector<std::string> &classes
    );
    TriggerAlerter(const TriggerAlerter &)=delete;
    TriggerAlerter(TriggerAlerter &&)=delete;

    // 更新所有区域
    void Update(std::shared_ptr<TrackerWorld> trackers, std::shared_ptr<cv::Mat> image);

    const std::unordered_set<size_t> &GetAlertClasses()const;

private:
    std::vector<Region> _regions;
    std::unordered_set<size_t> _trackClasses;
};