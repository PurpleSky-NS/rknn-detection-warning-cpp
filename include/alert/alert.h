// 结合追踪和区域实现多区域追踪
#pragma once
#include "alert/track/tracking.h"
#include "alert/region.h"

class LightAlerter
{
public:
    LightAlerter() = delete;
    LightAlerter(
        const std::vector<std::string> &regionsInfo, size_t w, size_t h, 
        const std::vector<std::string> &alertsInfo, const std::vector<std::string> &classes
    );
    LightAlerter(const LightAlerter &)=delete;
    LightAlerter(LightAlerter &&)=delete;

    void Update(std::shared_ptr<ResultType> result, std::shared_ptr<cv::Mat> image);

private:
    std::unique_ptr<Tracking> _tracking;
    std::vector<Region> _regions;

    // 不用OnEnter的原因是因为OnUpdate能同时完成这两个功能
    void OnLeave(STracker, const TrackerWorld&, std::shared_ptr<cv::Mat>);
    void OnUpdate(const TrackerWorld&, std::shared_ptr<cv::Mat>);
};

inline void LightAlerter::Update(std::shared_ptr<ResultType> result, std::shared_ptr<cv::Mat> image)
{
    _tracking->Update(*result, image);
}