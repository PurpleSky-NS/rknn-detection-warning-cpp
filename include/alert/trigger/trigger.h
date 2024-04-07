#pragma once

#include <spdlog/spdlog.h>
#include <httplib.h>
#include "alert/track/tracker.h"

class Trigger
{
public:
    
    /**
     * @brief 设置报警Url
     *
     * 设置触发器的报警Url。
     *
     * @param alertUrl 报警Url
     */
    static void SetAlertUrl(const std::string &alertUrl);

    /**
     * @brief 解析触发器信息并返回相应的触发器对象
     *
     * 根据给定的触发器信息字符串，解析其中的条件、事件、对象等信息，并返回相应的触发器对象。
     *
     * @param triggerInfo 触发器信息字符串
     *
     * @return 触发器对象
     * @throw std::invalid_argument 如果触发器解析失败
     */
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

private:
    inline static std::unique_ptr<httplib::Client> _alertClient;  // 报警客户端
    inline static std::string _alertPath;  // 报警路径
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