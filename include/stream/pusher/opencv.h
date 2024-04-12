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

    void PushFrame(std::shared_ptr<const cv::Mat> frame);
    OpencvPusher &operator<<(const cv::Mat &frame);

private:
    FILE *_ffProc;
    size_t _dataSize;
};
