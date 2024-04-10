#pragma once

#include <opencv2/opencv.hpp>
#include "stream/ffutils.h"


class OpencvPuller
{
public:
    OpencvPuller(const std::string &source, const std::string &resolution=std::string());
    OpencvPuller(const OpencvPuller&)=delete;
    OpencvPuller(OpencvPuller&&)=default;
    
    //有下面两种方式获取帧
    OpencvPuller &operator>>(cv::Mat &frame);
    std::shared_ptr<cv::Mat> GetFrame();

    //每次获取完帧之后，这个数会涨
    inline size_t GetFrameID()const;

    inline uint GetWidth()const;
    inline uint GetHeight()const;
    inline double GetFPS()const;

private:
    // 视频流相关变量
    std::unique_ptr<cv::VideoCapture> _cap;
    std::unique_ptr<FFmpegStreamer> _ffStreamer;  // 如果自带的opencv库有毛病，不能捕获网络流，那么就用这个ffmpeg拉流
    std::unique_ptr<FFmpegDecoder> _ffDecoder;
    uint _w, _h;
    uint _originW, _originH;
    double _fps;
    size_t _frameID;
    // 解析视频帧函数实现
    using GetFrameFn = void (OpencvPuller::*)(cv::Mat&);
    GetFrameFn _getFrameFn;

    void CheckStreamProp();

    bool InitFFmpegBackend(const std::string &source);
    void FFmpegParseFrame(cv::Mat &frame);

    bool InitOpencvBackend(const std::string &source);
    void OpencvParseFrame(cv::Mat &frame);
};

inline size_t OpencvPuller::GetFrameID()const
{
    return _frameID;
}

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