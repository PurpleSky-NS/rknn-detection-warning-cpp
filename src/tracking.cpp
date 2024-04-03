#include "alert/track/tracking.h"

#include <spdlog/spdlog.h>

Tracking::Tracking(const std::unordered_set<size_t> &trackClasses):
    _onEnterCallback([](auto,auto,auto){}),
    _onLeaveCallback([](auto,auto,auto){}),
    _onUpdateCallback([](auto,auto){})
{
    for(auto &cid: trackClasses)
        _trackers[cid];
    _existObjs = _trackers;
}

void Tracking::Update(const ResultType &results, std::shared_ptr<cv::Mat> image)
{
    if(_trackers.empty())
        return;
    auto time = std::chrono::steady_clock::now();

    // 这个存储进入/离开的动作以便最后统一更新，在中间更新的话回调拿到的全局物体信息是不对的
    std::vector<std::pair<STracker, Tracker::Action>> objAndAction;
    // 这个做遍历统计用
    TrackerWorld trackers = _trackers;
    for(const auto &obj: results){
        auto fd = trackers.find(obj.classIndex);
        if(fd == trackers.end())
            continue;
        auto maxSim = 0.;
        STracker maxSimObj;
        for(auto &tracker: fd->second){
            auto sim = tracker.second->CalcSim(obj.box);
            // 找最大位置相似度物体，并且必须大于最小位置相似度
            if(sim > maxSim && sim > _trackSimThreshhold){
                maxSim = sim;
                maxSimObj = tracker.second;
            }
        }
        if(maxSimObj){
            // 可以找到之前追踪的物体
            fd->second.erase(maxSimObj->GetID());
            if(auto oa = UpdateTracker(maxSimObj, maxSimObj->Update(time, obj.box)); oa)
                objAndAction.push_back(*oa);
        }
        else{
            // 那就新建一个物体追踪器
            STracker tracker = std::make_shared<Tracker>(obj);
            _trackers[obj.classIndex][tracker->GetID()] = tracker;
        }
    }

    // 没有被认领的物体空更新一次
    for(auto &[_, classes]: trackers)
        for(auto &[_, tracker]: classes)
            UpdateTracker(tracker, tracker->Update(time));
    
    // 到这里，所有的集合都更新完毕，下面进行回调
    for(const auto &[tracker, action]: objAndAction){
        switch (action)
        {
        case Tracker::ENTER:
            _onEnterCallback(tracker, _existObjs, image);
            break;
        case Tracker::LEAVE:
            _onLeaveCallback(tracker, _existObjs, image);
            break;
        default:
            break;
        }
    }
    _onUpdateCallback(_existObjs, image);
}
