#pragma once

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <string>


class CvShowPusher
{
public:
    CvShowPusher() = default;
    CvShowPusher(const CvShowPusher&)=delete;
    CvShowPusher(CvShowPusher&&);
    ~CvShowPusher() = default;

    void PushFrame(std::shared_ptr<const cv::Mat> frame)
    {
        *this << *frame;
    }

    CvShowPusher &operator<<(const cv::Mat &frame)
    {
        cv::imshow("123", frame);
        return *this;
    }

};