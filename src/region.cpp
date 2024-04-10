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

void Region::TriggerUpdate(std::shared_ptr<cv::Mat> image)
{
    for(auto &[classID, triggers]: _triggers){
        auto objs = _objects[classID];
        for(auto &trigger: triggers)
            trigger->Update(objs, image);
    }
}

// 更新单个画面中存在的物体
void Region::ObjectUpdate(STracker tracker, std::shared_ptr<cv::Mat> image)
{
    auto fd = _objects.find(tracker->GetObject().classIndex);
    if(fd == _objects.end())
        return;

    // 当该区域是全图区域或者该物体在该区域时
    const auto &pos = tracker->GetObject().box.GetPos();
    if(_region.empty() || cv::pointPolygonTest(_region, {static_cast<float>(pos.x), static_cast<float>(pos.y)}, false) == 1)
        Enter(tracker, image, fd->second);
    else
        Leave(tracker, image, fd->second);
}

void Region::ObjectLeave(STracker tracker, std::shared_ptr<cv::Mat> image)
{
    auto fd = _objects.find(tracker->GetObject().classIndex);
    if(fd == _objects.end())
        return;
    
    Leave(tracker, image, fd->second);
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
    trackerSet.erase(tracker->GetID());
    trackerSet[tracker->GetID()] = tracker;
    for(auto &trigger: _triggers[tracker->GetObject().classIndex])
        trigger->Leave(tracker, _objects[tracker->GetObject().classIndex], image);
}