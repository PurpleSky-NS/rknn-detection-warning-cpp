#pragma once
#include "alert/trigger/trigger.h"

class EnterTrigger: public Trigger
{
public:
    EnterTrigger(const std::string &event, const std::string &object, const std::string &region): Trigger(event, object, region) {}

    void Enter(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        Alert({tracker}, image);
    }
};