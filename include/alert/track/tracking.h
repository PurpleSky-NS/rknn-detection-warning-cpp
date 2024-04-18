#pragma once
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "alert/track/tracker.h"


class Tracking
{
public:
    // 设置追踪时间阈值
    static bool SetTrackSimThreshhold(double threshhold);

    using OnObjectChanges = std::function<void(STracker, const TrackerWorld&, std::shared_ptr<cv::Mat>)>;
    using OnObjectsUpdate = std::function<void(const TrackerWorld&, std::shared_ptr<cv::Mat>)>;

    Tracking() = default;
    Tracking(const std::unordered_set<size_t> &trackClasses);
    Tracking(const Tracking &) = default;
    Tracking(Tracking &&) = default;

    // 设置进入回调
    void SetEnterCallback(OnObjectChanges onEnterCallback);

    // 设置离开回调
    void SetLeaveCallback(OnObjectChanges onLeaveCallback);

    // 设置更新回调
    void SetUpdateCallback(OnObjectsUpdate onObjectsUpdateCallback);

    // 用识别结果更新
    void Update(const ResultType &results, std::shared_ptr<cv::Mat> image);

private:
    inline static double _trackSimThreshhold = 0.45;

    TrackerWorld _trackers;
    TrackerWorld _existObjs;
    OnObjectChanges _onEnterCallback, _onLeaveCallback;
    OnObjectsUpdate _onUpdateCallback;

    // 利用动作和tracker对象来更新
    std::optional<std::pair<STracker, Tracker::Action>> UpdateTracker(STracker tracker, Tracker::Action action);
};

// 设置进入回调
inline void Tracking::SetEnterCallback(OnObjectChanges onEnterCallback)
{
    _onEnterCallback = onEnterCallback;
}

// 设置离开回调
inline void Tracking::SetLeaveCallback(OnObjectChanges onLeaveCallback)
{
    _onLeaveCallback = onLeaveCallback;
}

// 设置更新回调
inline void Tracking::SetUpdateCallback(OnObjectsUpdate onUpdateCallback)
{
    _onUpdateCallback = onUpdateCallback;
}

inline std::optional<std::pair<STracker, Tracker::Action>> Tracking::UpdateTracker(STracker tracker, Tracker::Action action)
{
    // 处理物体集合
    switch (action)
    {
    case Tracker::ENTER:
        _existObjs[tracker->GetObject().classIndex][tracker->GetID()] = tracker;
        break;
    case Tracker::LEAVE:
        _existObjs[tracker->GetObject().classIndex].erase(tracker->GetID());
        _trackers[tracker->GetObject().classIndex].erase(tracker->GetID());
        break;
    case Tracker::FAKE:
        _trackers[tracker->GetObject().classIndex].erase(tracker->GetID());
        break;
    default:
        break;
    }
    switch (action)
    {
    case Tracker::ENTER:
    case Tracker::LEAVE:
        return std::make_pair(tracker, action);
    }
    return std::nullopt;
}