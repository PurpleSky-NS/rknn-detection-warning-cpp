#include "alert/alert.h"

#include <json/json.h>
#include <spdlog/spdlog.h>

TriggerAlerter::TriggerAlerter(
    const std::vector<std::string> &regionsInfo, size_t w, size_t h, 
    const std::vector<std::string> &alertsInfo, const std::vector<std::string> &classes
)
{
    std::unordered_map<std::string, size_t> regionMap, classMap;

    for(size_t i=0; i<classes.size(); ++i)
        classMap[classes[i]] = i;

    Json::Reader reader;
	Json::Value root;

	// 解析区域Json
    for(auto &regionInfo: regionsInfo){
        if(!reader.parse(regionInfo, root)){
            spdlog::critical("解析区域Json字符串失败，区域配置为：[{}]", regionInfo);
            throw std::invalid_argument("报警器构造失败");
        }
        auto name = root["name"].asString();
        std::vector<cv::Point2f> regionPoints;
        cv::Point2f point;
        auto &region = root["region"];
        // 区域的信息为归一化的x1,y1,x2,y2...
        for(int i=0; i<region.size(); ++i){
            if(i%2){
                point.y = region[i].asFloat() * h;
                regionPoints.push_back(point);
            }
            else
                point.x = region[i].asFloat() * w;
        }
        regionMap[name] = _regions.size();
        _regions.emplace_back(name, regionPoints);
    }
    regionMap[""] = _regions.size();
    _regions.push_back(Region());
    
    // 解析报警Json
    for(auto &alertInfo: alertsInfo){
        auto trigger = Trigger::ParseTrigger(alertInfo);
        auto regionFd = regionMap.find(trigger->GetRegion());
        if(regionFd == regionMap.end()){
            spdlog::critical("事件触发器构造失败，事件[{}]所指定的区域[{}]不存在", trigger->GetEvent(), trigger->GetRegion());
            throw std::invalid_argument("报警器构造失败");
        }
        auto classFd = classMap.find(trigger->GetObject());
        if(classFd == classMap.end()){
            spdlog::critical("事件触发器构造失败，事件[{}]所指定的物体[{}]不存在", trigger->GetEvent(), trigger->GetObject());
            throw std::invalid_argument("报警器构造失败");
        }
        _regions[regionFd->second].AddTrigger(classFd->second, trigger);
        _trackClasses.insert(classFd->second);
    }
    for(auto i=_regions.begin(); i!=_regions.end();)
        if(i->Empty())
            i = _regions.erase(i);
        else
            ++i;
    _regions.shrink_to_fit();
    spdlog::debug("事件触发器构造结束，共计{}有效区域", _regions.size());

    if(!_trackClasses.empty())
        spdlog::debug(
            "追踪器已构建，将对画面中的[{}]进行追踪", 
            std::accumulate(
                ++_trackClasses.begin(), _trackClasses.end(), 
                classes[*_trackClasses.begin()], 
                [&classes](auto &s, auto val){return s + (", " + classes[val]);}
            )
        );
}

void TriggerAlerter::Update(std::shared_ptr<TrackerWorld> trackerWorld, std::shared_ptr<cv::Mat> image)
{
    for(auto &region: _regions)
        region.Update(trackerWorld, image);
}

const std::unordered_set<size_t> &TriggerAlerter::GetAlertClasses()const
{
    return _trackClasses;
}