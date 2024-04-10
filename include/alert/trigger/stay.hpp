#pragma once
#include "alert/trigger/trigger.h"

class StayTrigger: public Trigger
{
    StayTrigger(const std::string &event, const std::string &object, const std::string &region, const std::string &condition, float threshhold): 
        Trigger(event, object, region), _threshhold(threshhold * 1000)  // s -> ms
    {
        if(threshhold <= 0){
            spdlog::critical("时间阈值必须大于0，传入时间为：[{}]", threshhold);
            throw std::invalid_argument("经过触发器构造失败");
        }
    }

    void Enter(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        _stayTrackers[tracker->GetID()] = tracker;
    }

    void Update(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        for(auto &[_, stayTracker]: _stayTrackers){
            if(std::chrono::duration_cast<std::chrono::milliseconds>(stayTracker->GetCurrentTime() - stayTracker->GetCreateTime()).count() >= _threshhold){
                Alert({stayTracker}, stayTracker->GetImage());
                _stayTrackers.erase(tracker->GetID());  // 删除已经离开的物体（这个操作应该不会使迭代器失效）
            }
        }
    }

    void Leave(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        _stayTrackers.erase(tracker->GetID());
    }

private:
    uint _threshhold;  // 停留时间阈值
    std::unordered_map<std::string, STracker> _stayTrackers;  // 存储所有目前留在画面上的物体
};