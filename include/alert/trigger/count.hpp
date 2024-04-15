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

    void Update(const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        if(_judgment(trackers.size())){
            if(_alerted)
                return;
            _alerted = true;
            std::vector<STracker> alertObjs;
            alertObjs.reserve(trackers.size());
            for(auto &[_, obj]: trackers)
                alertObjs.push_back(obj);
            Alert(alertObjs, image);
        }
        else
            _alerted = false;
    }

private:
    bool _alerted = false;
    std::function<bool(uint)> _judgment;
};