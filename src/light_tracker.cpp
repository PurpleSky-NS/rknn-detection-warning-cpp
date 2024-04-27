#include "tracking/light/tracker.h"
#include <spdlog/spdlog.h>

bool LightTracker::SetTrackTimeThreshhold(std::chrono::milliseconds ms)
{
    if(!ms.count())
        return false;
    _trackTimeThreshhold = ms;
    return true;
}

bool LightTracker::SetTrackEnterPercentThreshhold(double percent)
{
    if(percent < 0.0 || percent > 1.0)
        return false;
    _trackEnterPercentThreshhold = percent;
    return true;
}

bool LightTracker::SetTrackLeavePercentThreshhold(double percent)
{
    if(percent < 0.0 || percent > 1.0)
        return false;
    _trackLeavePercentThreshhold = percent;
    return true;
}

LightTracker::LightTracker(const Object &obj, size_t id): 
    Tracker(obj, id),
    _lastTrackTime(_createTime),
    _exTrackCount(1), 
    _totalTrackCount(1)
{}

double LightTracker::CalcSim(const Box &box)const
{
    double xmin1 = _obj.box.x, ymin1 = _obj.box.y, w1 = _obj.box.w, h1 = _obj.box.h;
    double xmin2 = box.x, ymin2 = box.y, w2 = box.w, h2 = box.h;
    double xmax1 = xmin1 + w1, ymax1 = ymin1 + h1;
    double xmax2 = xmin2 + w2, ymax2 = ymin2 + h2;

    double w = std::max(xmax1, xmax2) - std::min(xmin1, xmin2), h = std::max(ymax1, ymax2) - std::min(ymin1, ymin2);
    double wp2 = w * w, hp2 = h * h;
    if(w <= 0 || h <= 0)
        return 0;

    double inter = std::max(0., std::min(xmax1, xmax2) - std::max(xmin1, xmin2)) * std::max(0., std::min(ymax1, ymax2) - std::max(ymin1, ymin2));
    double unio = w1 * h1 + w2 * h2 - inter;
    double iou = 0;
    if(inter > 0 && unio > 0)
        iou = inter / unio;

    // 形状相似度
    double wdis = w1 - w2, hdis = h1 - h2;
    double shape_sim = 1 - (wdis * wdis / wp2 + hdis * hdis / hp2) / 2;

    // 中心点距离相似度（0~1）
    double sum = wp2 + hp2;
    double xsum = (xmin1 + xmax1) / 2 - (xmin2 + xmax2) / 2, ysum = (ymin1 + ymax1) / 2 - (ymin2 + ymax2) / 2;
    double dis_sim = 1 - (xsum * xsum + ysum * ysum) / sum;

    return iou * 0.5 + shape_sim * 0.3 + dis_sim * 0.2;
}

LightTracker::Action LightTracker::Update(std::chrono::steady_clock::time_point time)
{
    _currentTime = time;
    ++_totalTrackCount;

    if(time - _lastTrackTime > _trackTimeThreshhold){ // 当经过一次追踪检测之后，根据当前状态更新动作
        _lastTrackTime = time;
        // 计算检测比
        auto trackPercent = _totalTrackCount ? static_cast<double>(_exTrackCount) / _totalTrackCount : 0.;
        // 给计数器置零
        _exTrackCount = _totalTrackCount = 0;

        if(!_exists){
            if(trackPercent >= _trackEnterPercentThreshhold){
                // 判定进入时，需要达到进入检测比阈值
                _exists = true;
                spdlog::debug("[({}){}] 判定进入，检测比为: {}", _obj.className, _id, trackPercent);
                return ENTER;
            }
            else{
                spdlog::debug("[({}){}] 判定抖动，检测比为: {}", _obj.className, _id, trackPercent);
                return FAKE;
            }
        }
        else if(trackPercent <= _trackLeavePercentThreshhold){  // 能走这个elif，肯定是existed=True
            // 判定离开时，需要小于离开检测比阈值，离开对检测比的判定稍微宽松点，因为即使只有一次检测到了，也能证明物体存在
            spdlog::debug("[({}){}] 判定离开，检测比为: {}", _obj.className, _id, trackPercent);
            return LEAVE;
        }
    }
    return NONE;
}

LightTracker::Action LightTracker::Update(std::chrono::steady_clock::time_point time, const Object &obj, std::shared_ptr<cv::Mat> image)
{
    _obj = obj;
    _lastTrackImage = image;
    ++_exTrackCount;
    if(_trajectory.size() == _maxTrajectoryCount)
        _trajectory.pop_front();
    _trajectory.push_back(obj.box.GetPos());
    return Update(time);
}