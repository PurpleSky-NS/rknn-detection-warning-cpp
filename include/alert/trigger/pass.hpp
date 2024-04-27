#pragma once
#include "alert/trigger/trigger.h"

class PassTrigger: public Trigger
{
public:
    enum Direction{
        UP = 0,
        RIGHT = 1,
        DOWN = 2,
        LEFT = 3
    };

    PassTrigger(const std::string &event, const std::string &object, const std::string &region, const std::string &condition): 
        Trigger(event, object, region)
    {
        if(condition == "向上经过")
            _direction = UP;
        else if(condition == "向下经过")
            _direction = DOWN;
        else if(condition == "向左经过")
            _direction = LEFT;
        else if(condition == "向右经过")
            _direction = RIGHT;
        else
            spdlog::critical("不支持该经过条件：{}", condition);
    }

    void Leave(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        auto enterPos = tracker->GetEnterPoint();
        auto leavePos = tracker->GetObject().box.GetPos();
        auto alert = false;
        if(_direction == UP)
            alert = leavePos.y - enterPos.y < 0;
        else if(_direction == DOWN)
            alert = leavePos.y - enterPos.y > 0;
        else if(_direction == LEFT)
            alert = leavePos.x - enterPos.x < 0;
        else if(_direction == RIGHT)
            alert = leavePos.x - enterPos.x > 0;
        if(alert)
            Alert({tracker}, tracker->GetImage());
    }
private:
    Direction _direction;
};