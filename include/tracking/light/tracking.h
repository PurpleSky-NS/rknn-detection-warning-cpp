#pragma once
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>
#include "tracking/light/tracker.h"

class LightTracking
{
public:
    // 设置追踪时间阈值
    static bool SetTrackSimThreshhold(double threshhold);

    LightTracking() = default;
    LightTracking(const std::unordered_set<size_t> &trackClasses);
    LightTracking(const LightTracking &) = default;
    LightTracking(LightTracking &&) = default;

    // 用识别结果更新，并返回当前的所有追踪器
    std::shared_ptr<TrackerWorld> Update(std::shared_ptr<ResultType> results, std::shared_ptr<cv::Mat> image);

    // // 获取所有追踪器
    // const TrackerWorld &GetTrackers() const;

    // // 获取所有当前画面中存在的物体
    // const TrackerWorld &GetExistObjs() const;

private:
    inline static double _trackSimThreshhold = 0.45;

    TrackerWorld _trackers;
    // TrackerWorld _existObjs;
    std::unordered_map<size_t, size_t> _trackerIDs;

    // 利用动作和tracker对象来更新
    void UpdateTracker(SLightTracker tracker, LightTracker::Action action);
};

inline void LightTracking::UpdateTracker(SLightTracker tracker, LightTracker::Action action)
{
    // 处理物体集合
    switch (action)
    {
    // case Tracker::ENTER:
    //     _existObjs[tracker->GetObject().classIndex][tracker->GetID()] = tracker;
    //     break;
    case LightTracker::LEAVE:
    case LightTracker::FAKE:
        // _existObjs[tracker->GetObject().classIndex].erase(tracker->GetID());
        _trackers[tracker->GetObject().classIndex].erase(tracker->GetID());
        break;
    }
}

// 获取所有当前画面中存在的物体
// const TrackerWorld &LightTracker::GetExistObjs()const
// {
//     return _existObjs;
// }