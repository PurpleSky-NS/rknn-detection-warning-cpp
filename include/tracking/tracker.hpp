#pragma once
#include <string>
#include <chrono>
#include <queue>
#include <ulid/ulid.hh>
#include <spdlog/spdlog.h>
#include "type.h"

// 把下面该返回的都实现了，报警和绘图就能正常运行
class Tracker
{
public:
    Tracker() = delete;
    Tracker(const Object &obj, size_t id);  // 需要追踪时刻的位置，以及分配的短ID来构造
    Tracker(const Tracker &) = default;
    Tracker(Tracker &&) = delete;

    // 获取关联的Object对象
    const Object &GetObject()const;

    // 获取物体进入时的位置
    const Box &GetEnterBox()const;

    // 获取追踪轨迹
    const std::deque<Point> &GetTrajectory()const;

    // 获取ID
    size_t GetID()const;

    // 获取UID（全球唯一，自动生成）
    const std::string &GetUID()const;

    // 判断物体是否已经被判定为进入
    bool Exists()const;

    // 获取该物体所在的图像
    std::shared_ptr<const cv::Mat> GetImage()const;

    // 获取物体创建时间
    const std::chrono::steady_clock::time_point &GetCreateTime()const;

    // 获取物体当前时间
    const std::chrono::steady_clock::time_point &GetCurrentTime()const;
    
protected:   
    Object _obj;
    size_t _id;  // 追踪ID，由追踪系统分配
    std::string _ulid;  // 追踪ID，使用ULID生成
    bool _exists;
    std::chrono::steady_clock::time_point _createTime, _currentTime;
    std::shared_ptr<cv::Mat> _lastTrackImage;
    Box _enterBox;  // 物体进入时的位置
    std::deque<Point> _trajectory;  // 追踪轨迹
};

using STracker = std::shared_ptr<Tracker>;
using TrackerSet = std::unordered_map<size_t, STracker>;
using TrackerWorld = std::unordered_map<size_t, TrackerSet>;  // Tracker可以被组织为：{类别ID: [{ID1: 物体1}, {ID2: 物体2}...]}

inline Tracker::Tracker(const Object &obj, size_t id): 
    _obj(obj),
    _id(id),
    _ulid(ulid::Marshal(ulid::CreateNowRand())),
    _exists(false),
    _createTime(std::chrono::steady_clock::now()),
    _currentTime(_createTime),
    _enterBox(obj.box)
{
    spdlog::debug("[({}){}] 开始追踪", _obj.className, _id);
}

inline const Object &Tracker::GetObject()const
{
    return _obj;
}

inline const Box &Tracker::GetEnterBox()const
{
    return _enterBox;
}

inline const std::deque<Point> &Tracker::GetTrajectory()const
{
    return _trajectory;
}

inline size_t Tracker::GetID()const
{
    return _id;
}

inline const std::string &Tracker::GetUID()const
{
    return _ulid;
}

inline bool Tracker::Exists()const
{
    return _exists;
}

inline std::shared_ptr<const cv::Mat> Tracker::GetImage()const
{
    return _lastTrackImage;
}

inline const std::chrono::steady_clock::time_point &Tracker::GetCreateTime()const
{
    return _createTime;
}

inline const std::chrono::steady_clock::time_point &Tracker::GetCurrentTime()const
{
    return _currentTime;
}