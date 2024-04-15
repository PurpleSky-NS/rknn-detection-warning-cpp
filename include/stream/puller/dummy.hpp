#pragma once

#include <opencv2/opencv.hpp>
#include <thread>


class DummyPuller
{
public:
    DummyPuller(const std::string &imagePath);
    DummyPuller(const DummyPuller&)=delete;
    DummyPuller(DummyPuller&&)=default;
    
    //有下面两种方式获取帧
    DummyPuller &operator>>(cv::Mat &frame);
    std::shared_ptr<cv::Mat> Fetch();

    //每次获取完帧之后，这个数会涨
    inline size_t GetFrameID()const;

    inline uint GetWidth()const;
    inline uint GetHeight()const;
    inline double GetFPS()const;

private:
    cv::Mat _image;
    size_t _frameID;
    uint _w, _h;
};

inline DummyPuller::DummyPuller(const std::string &imagePath)
{
    _image = cv::imread(imagePath);
    auto [w, h] = _image.size();
    _w = w;
    _h = h;
}

DummyPuller &DummyPuller::operator>>(cv::Mat &frame)
{
    frame = _image.clone();
    ++_frameID;
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint>(1000 / GetFPS())));
    return *this;
}

std::shared_ptr<cv::Mat> DummyPuller::Fetch()
{
    cv::Mat img;
    *this >> img;
    return std::make_shared<cv::Mat>(img);
}

inline size_t DummyPuller::GetFrameID()const
{
    return _frameID;
}

inline uint DummyPuller::GetWidth()const
{
    return _w;
}

inline uint DummyPuller::GetHeight()const
{
    return _h;
}

inline double DummyPuller::GetFPS()const
{
    return 25;
}