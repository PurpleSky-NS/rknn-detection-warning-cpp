#include "stream/puller/opencv.h"
#include <spdlog/spdlog.h>
#include <cctype>

OpencvPuller::OpencvPuller(const std::string &source)
{
    if(std::all_of(source.begin(), source.end(), [](const auto &ch){return isdigit(ch);}))
        _cap.open(std::stoi(source));
    else
        _cap.open(source);
    if(!_cap.isOpened()){
        spdlog::critical("拉取视频流[{}]失败", source);
        throw std::runtime_error("拉取视频流失败");
    }
    _w = static_cast<uint>(_cap.get(cv::CAP_PROP_FRAME_WIDTH));
    _h = static_cast<uint>(_cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    _fps = _cap.get(cv::CAP_PROP_FPS);
    if(_w * _h * _fps == 0){
        spdlog::critical("视频流属性获取失败");
        throw std::runtime_error("拉流失败");
    }
    if(_fps > 120){
        spdlog::critical("检测到帧率[{}]过高，判定为拉流失败", _fps);
        throw std::runtime_error("拉流失败");
    }
}

OpencvPuller &OpencvPuller::operator>>(cv::Mat &frame)
{
    frame.release();
    while(frame.empty())
        _cap >> frame;
    ++_frameID;
    return *this;
}

std::shared_ptr<cv::Mat> OpencvPuller::Pull()
{
    cv::Mat frame;
    *this >> frame;
    return std::make_shared<cv::Mat>(frame);
}

// void OpencvPuller::FFmpegParseFrame(cv::Mat &frame)
// {
//     while(true){
//         auto pkt = _pktPuller->Pull();
//         if(pkt){
//             auto data = _softDecoder->Decode(pkt);
//             if(data){
//                 frame = std::move(cv::Mat(_originH, _originW, CV_8UC3, data));
//                 break;
//             }
//         }
//     }
// }