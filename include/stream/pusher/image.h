#pragma once

#include <opencv2/opencv.hpp>
#include <string>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

class ImagePusher
{
public:
    ImagePusher(const std::string &output, const cv::Size &size, double fps, uint pusherCBR=600*1024*8, uint thread=1, bool h265=false, bool lowLatency=false);
    ImagePusher(const ImagePusher&)=delete;
    ImagePusher(ImagePusher&&)=delete;
    ~ImagePusher();

    void operator()(std::shared_ptr<cv::Mat> frame);

private:
    AVFormatContext *_outputFmtCtx = nullptr;
    AVCodecContext* _codecCtx = nullptr;
    AVStream* _videoStream = nullptr;
    SwsContext* _swsCtx = nullptr;
    AVFrame* _frameYUV = nullptr;
    int64_t _vpts = 0;
};
