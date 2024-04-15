#pragma once

#include <opencv2/opencv.hpp>
#include "stream/puller/packet.h"
#include "stream/decoder/soft.h"


class OpencvPuller
{
public:
    OpencvPuller()=delete;
    OpencvPuller(const std::string &source);
    OpencvPuller(const OpencvPuller&)=default;
    OpencvPuller(OpencvPuller&&)=default;
    
    //有下面两种方式获取帧
    OpencvPuller &operator>>(cv::Mat &frame);
    std::shared_ptr<cv::Mat> Pull();

    inline uint GetWidth()const;
    inline uint GetHeight()const;
    inline double GetFPS()const;

private:
    // 视频流相关变量
    cv::VideoCapture _cap;
    uint _w, _h;
    double _fps;
    size_t _frameID;
};

inline uint OpencvPuller::GetWidth()const
{
    return _w;
}

inline uint OpencvPuller::GetHeight()const
{
    return _h;
}

inline double OpencvPuller::GetFPS()const
{
    return _fps;
}