#include "alert/region.h"

#include <spdlog/spdlog.h>

Region::Region()
{
    spdlog::debug("已添加区域[整个画面]");
}

Region::Region(const std::string &name, const std::vector<cv::Point2f> &region):
    _name(name), _region(region)
{
    if(region.size() < 3){
        spdlog::critical("区域[{}]构建失败，无法使用{}个点组成一个多边形", name, region.size());
        throw std::invalid_argument("区域构建失败");
    }
    spdlog::debug("已添加区域[{}]，该区域由{}个点围成", name, region.size());
}

void Region::AddTrigger(size_t classID, STrigger trigger)
{
    _triggers[classID].push_back(trigger);
    _triggers[classID].shrink_to_fit();
    _objects[classID];
}

bool Region::Empty()const
{
    return _triggers.empty();
}

void Region::Update(std::shared_ptr<TrackerWorld> trackerWorld, std::shared_ptr<cv::Mat> image)
{
    for(auto &[classID, trackers]: *trackerWorld){
        // 一类一类判断，如果这个类不在这个区域的追踪范围则直接忽略
        auto fd = _objects.find(classID);
        if(fd == _objects.end())
            continue;
        auto trackerSet = fd->second;  // 建立个not found集合，剩下的物体都是not found，算离开
        for(auto &[id, tracker]: trackers){
            if(!tracker->Exists())
                continue;
            // 当该区域是全图区域或者该物体在该区域时
            const auto &pos = tracker->GetObject().box.GetPos();
            if(_region.empty() || cv::pointPolygonTest(_region, {static_cast<float>(pos.x), static_cast<float>(pos.y)}, false) == 1)
                Enter(tracker, image, fd->second);
            else
                Leave(tracker, image, fd->second);
            trackerSet.erase(id);  // 不算not found了
        }
        for(auto &[_, tracker]: trackerSet)
            Leave(tracker, image, fd->second);  // 剩下的都是not found，算离开
    }
    // 进入/离开都更新完了，更新触发器的Update
    for(auto &[classID, triggers]: _triggers){
        auto objs = _objects[classID];
        for(auto &trigger: triggers)
            trigger->Update(objs, image);
    }
}
    
void Region::Enter(STracker tracker, std::shared_ptr<cv::Mat> image, TrackerSet &trackerSet)
{
    auto fd = trackerSet.find(tracker->GetID());
    if(fd != trackerSet.end())
        return;
    // 有新物体进入该区域
    spdlog::debug("({})物体[{}]进入区域[{}]", tracker->GetID(), tracker->GetObject().className, _name);
    trackerSet[tracker->GetID()] = tracker;
    for(auto &trigger: _triggers[tracker->GetObject().classIndex])
        trigger->Enter(tracker, _objects[tracker->GetObject().classIndex], image);
}

void Region::Leave(STracker tracker, std::shared_ptr<cv::Mat> image, TrackerSet &trackerSet)
{
    auto fd = trackerSet.find(tracker->GetID());
    if(fd == trackerSet.end())
        return;
    // 有物体离开该区域
    spdlog::debug("({})物体[{}]离开区域[{}]", tracker->GetID(), tracker->GetObject().className, _name);
    trackerSet.erase(fd);
    for(auto &trigger: _triggers[tracker->GetObject().classIndex])
        trigger->Leave(tracker, _objects[tracker->GetObject().classIndex], image);
}