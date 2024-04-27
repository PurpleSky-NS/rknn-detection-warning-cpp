#pragma once
#include "tracking/tracker.hpp"

class LightTracker: public Tracker
{
public:
    enum Action: unsigned char{
        NONE = 0,
        ENTER = 1,
        LEAVE = 2,
        FAKE = 3
    };
    // 设置追踪时间阈值
    static bool SetTrackTimeThreshhold(std::chrono::milliseconds time);

    // 设置追踪进入阈值
    static bool SetTrackEnterPercentThreshhold(double percent);

    // 设置追踪离开阈值
    static bool SetTrackLeavePercentThreshhold(double percent);

    LightTracker() = delete;
    LightTracker(const Object &obj, size_t id);  // 需要追踪时刻的位置，以及分配的短ID来构造
    LightTracker(const LightTracker &) = default;
    LightTracker(LightTracker &&) = delete;

    // 计算相似度
    double CalcSim(const Box &box)const;

    // 更新追踪器的状态
    Action Update(std::chrono::steady_clock::time_point time);
    Action Update(std::chrono::steady_clock::time_point time, const Object &obj, std::shared_ptr<cv::Mat> image);

protected:   
    // 追踪时间阈值，每过一次这个时间会确认一次物体是否停留在画面中
    inline static std::chrono::duration _trackTimeThreshhold = std::chrono::milliseconds(1000);
    // 认为物体在画面中的检测比
    inline static double _trackEnterPercentThreshhold = 0.6;
    // 认为物体不在画面中的检测比（设为0表示只要物体在一次时间阈值中任意出现在画面中一次即可）
    inline static double _trackLeavePercentThreshhold = 0.0;
    // 最多轨迹存储数量
    inline static size_t _maxTrajectoryCount = 200;

    std::chrono::steady_clock::time_point _lastTrackTime;
    size_t _exTrackCount, _totalTrackCount;
};

using SLightTracker = std::shared_ptr<LightTracker>;