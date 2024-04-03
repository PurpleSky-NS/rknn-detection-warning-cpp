#pragma once

#include <spdlog/spdlog.h>
#include "alert/track/tracker.h"

class Trigger
{
public:
    static std::shared_ptr<Trigger> ParseTrigger(const std::string &triggerInfo);

    Trigger(const std::string &event, const std::string &object, const std::string &region);
    Trigger() = delete;
    Trigger(const Trigger &) = default;
    Trigger(Trigger &&) = delete;
    virtual ~Trigger() = default;

    virtual void Enter(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image){}
    virtual void Leave(STracker tracker, const TrackerSet &trackers, std::shared_ptr<cv::Mat> image){}
    virtual void Update(const TrackerSet &trackers, std::shared_ptr<cv::Mat> image){}

    const std::string GetEvent()const;
    const std::string GetObject()const;
    const std::string GetRegion()const;

protected:
    std::string _event, _object, _region;
    
    // 调用这个接口报警
    void Alert(const std::vector<STracker> &objs, std::shared_ptr<cv::Mat> image);
};
using STrigger = std::shared_ptr<Trigger>;

inline const std::string Trigger::GetEvent()const
{
    return _event;
}

inline const std::string Trigger::GetObject()const
{
    return _object;
}

inline const std::string Trigger::GetRegion()const
{
    return _region;
}