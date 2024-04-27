#include "tracking/light/tracking.h"
#include <spdlog/spdlog.h>

bool LightTracking::SetTrackSimThreshhold(double threshhold)
{
    if(_trackSimThreshhold <= 0)
        return false;
     _trackSimThreshhold = threshhold;
     return true;
}

LightTracking::LightTracking(const std::unordered_set<size_t> &trackClasses)
{
    for(auto &cid: trackClasses){
        _trackers[cid];
        _trackerIDs[cid] = 0;  // 物体ID从0开始
    }
    // _existObjs = _trackers;
}

std::shared_ptr<TrackerWorld> LightTracking::Update(std::shared_ptr<ResultType> results, std::shared_ptr<cv::Mat> image)
{
    if(_trackers.empty())
        return std::make_shared<TrackerWorld>(_trackers);
    auto time = std::chrono::steady_clock::now();

    // 这个做遍历统计用
    TrackerWorld trackers = _trackers;
    for(const auto &obj: *results){
        auto fd = trackers.find(obj.classIndex);
        if(fd == trackers.end())
            continue;
        auto maxSim = 0.;
        SLightTracker maxSimObj;
        for(auto &[id, btracker]: fd->second){
            auto tracker = std::static_pointer_cast<LightTracker>(btracker);
            auto sim = tracker->CalcSim(obj.box);
            // 找最大位置相似度物体，并且必须大于最小位置相似度
            if(sim > maxSim && sim >= _trackSimThreshhold){
                maxSim = sim;
                maxSimObj = tracker;
            }
        }
        if(maxSimObj){
            // 可以找到之前追踪的物体
            fd->second.erase(maxSimObj->GetID());
            UpdateTracker(maxSimObj, maxSimObj->Update(time, obj, image));
        }
        else{
            // 那就新建一个物体追踪器
            STracker tracker = std::make_shared<LightTracker>(obj, _trackerIDs[obj.classIndex]++);
            _trackers[obj.classIndex][tracker->GetID()] = tracker;
        }
    }

    // 没有被认领的物体空更新一次
    for(auto &[_, classes]: trackers)
        for(auto &[_, tracker]: classes)
            UpdateTracker(
                std::static_pointer_cast<LightTracker>(tracker), 
                std::static_pointer_cast<LightTracker>(tracker)->Update(time)
            );

    return std::make_shared<TrackerWorld>(_trackers);
}
