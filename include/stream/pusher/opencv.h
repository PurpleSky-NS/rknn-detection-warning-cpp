#pragma once

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <string>


class OpencvPusher
{
public:
    OpencvPusher(const std::string &output, uint w, uint h, double fps);
    OpencvPusher(const OpencvPusher&)=delete;
    OpencvPusher(OpencvPusher&&);
    ~OpencvPusher();

    void PushFrame(const cv::Mat &frame);
    OpencvPusher &operator<<(const cv::Mat &frame);

    inline uint GetWidth()const;
    inline uint GetHeight()const;
    inline double GetFPS()const;

private:
    uint _w, _h;
    double _fps;
    FILE *_ffProc;
    size_t _dataSize;
};

inline uint OpencvPusher::GetWidth()const
{
    return _w;
}

inline uint OpencvPusher::GetHeight()const
{
    return _h;
}

inline double OpencvPusher::GetFPS()const
{
    return _fps;
}