#pragma once

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <string>


class FFmpegPusher
{
public:
    FFmpegPusher(const std::string &output, uint w, uint h, double fps);
    FFmpegPusher(const FFmpegPusher&)=delete;
    FFmpegPusher(FFmpegPusher&&)=delete;
    ~FFmpegPusher();

    void Push(std::shared_ptr<const cv::Mat> frame);
    FFmpegPusher &operator<<(const cv::Mat &frame);

private:
    FILE *_ffProc;
    size_t _dataSize;
};
