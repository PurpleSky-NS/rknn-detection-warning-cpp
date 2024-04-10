#pragma once
#include <functional>
#include "alert/trigger/trigger.h"

class CountTrigger: public Trigger
{
public:
    CountTrigger(const std::string& event, const std::string& object, const std::string& region, const std::string& op, uint threshhold):
        Trigger(event, object, region)
    {
        if(op == "等于") 
            _judgment = [threshhold](uint count) -> bool {return count == threshhold;};
        else if(op == "不等于")
            _judgment = [threshhold](uint count) -> bool {return count != threshhold;};
        else if(op == "大于")
            _judgment = [threshhold](uint count) -> bool {return count > threshhold;};
        else if(op == "大于等于")
            _judgment = [threshhold](uint count) -> bool {return count >= threshhold;};
        else if(op == "小于")
            _judgment = [threshhold](uint count) -> bool {return count < threshhold;};
        else if(op == "小于等于")
            _judgment = [threshhold](uint count) -> bool {return count <= threshhold;};
    }

    void Enter(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        TestAlert(trackers, image);
    }

    void Leave(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        TestAlert(trackers, image);
    }

private:
    bool _alerted = false;
    std::function<bool(uint)> _judgment;

    /**
     * @brief 测试警告函数
     *
     * 根据给定的目标集合和图像，判断是否需要触发警告。
     *
     * @param objs 目标集合
     * @param image 图像
     */
    void TestAlert(const TrackerSet& objs, std::shared_ptr<cv::Mat> image)
    {
        if(_judgment(objs.size())){
            if(_alerted)
                return;
            _alerted = true;
            std::vector<STracker> alertObjs;
            alertObjs.reserve(objs.size());
            for(auto &[_, obj]: objs)
                alertObjs.push_back(obj);
            Alert(alertObjs, image);
        }
        else
            _alerted = false;
    }
};