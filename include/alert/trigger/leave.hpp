#pragma once
#include "alert/trigger/trigger.h"

class LeaveTrigger: public Trigger
{
public:
    LeaveTrigger(const std::string &event, const std::string &object, const std::string &region): Trigger(event, object, region) {}

    void Leave(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image)
    {
        Alert({tracker}, tracker->GetImage());
    }
};