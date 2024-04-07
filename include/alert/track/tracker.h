#pragma once
#include <string>
#include <chrono>
#include <ulid/ulid.hh>
#include <opencv2/opencv.hpp>
#include "type.h"


class Tracker
{
public:
    enum Action: unsigned char{
        NONE = 0,
        ENTER = 1,
        LEAVE = 2,
        FAKE = 3
    };
    Tracker() = delete;
    Tracker(const Object &obj);
    Tracker(const Tracker &) = default;
    Tracker(Tracker &&) = delete;

    // 获取关联的Object对象
    const Object &GetObject()const;

    // 获取ID
    const std::string &GetID()const;

    // 获取该物体所在的图像
    std::shared_ptr<const cv::Mat> GetImage()const;
    
    // 计算相似度
    double CalcSim(const Box &box);

    // 获取中心点
    Point GetPos()const;

    // 更新追踪器的状态
    Action Update(std::chrono::steady_clock::time_point time);
    Action Update(std::chrono::steady_clock::time_point time, const Box &box, std::shared_ptr<cv::Mat> image);

private:
    inline static std::chrono::duration _trackTimeThreshhold = std::chrono::milliseconds(1000);
    inline static double _trackEnterPercentThreshhold = 0.6;
    inline static double _trackLeavePercentThreshhold = 0.0;

    Object _obj;
    std::string _id;
    bool _exists;
    std::chrono::steady_clock::time_point _createTime, _currentTime, _lastTrackTime;
    size_t _exTrackCount, _totalTrackCount;
    std::shared_ptr<cv::Mat> _lastTrackImage;
};

using STracker = std::shared_ptr<Tracker>;
using TrackerSet = std::unordered_map<std::string, STracker>;
using TrackerWorld = std::unordered_map<size_t, TrackerSet>;  // Tracker可以被组织为：{类别ID: [{ID1: 物体1}, {ID2: 物体2}...]}

inline const Object &Tracker::GetObject()const
{
    return _obj;
}

inline const std::string &Tracker::GetID()const
{
    return _id;
}

inline std::shared_ptr<const cv::Mat> Tracker::GetImage()const
{
    return _lastTrackImage;
}

inline Point Tracker::GetPos()const
{
    return {_obj.box.x + static_cast<int>(_obj.box.w) / 2, _obj.box.y + static_cast<int>(_obj.box.h) / 2};
}