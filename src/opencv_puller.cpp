#include "stream/puller/opencv.h"

#include <cctype>

OpencvPuller::OpencvPuller(const std::string &source): _frameID(0)
{
    if(InitFFmpegBackend(source))
        _getFrameFn = &OpencvPuller::FFmpegParseFrame;
    else if(InitOpencvBackend(source))
        _getFrameFn = &OpencvPuller::OpencvParseFrame;
    else
        throw std::runtime_error("拉取视频流失败");
    CheckStreamProp();
}

void OpencvPuller::CheckStreamProp()
{
    if(_w * _h * _fps == 0){
        std::cerr << "视频流属性获取失败" <<std::endl;
        throw std::runtime_error("拉流失败");
    }
    if(_fps > 120){
        std::cerr << "检测到帧率[" << _fps << "]过高，判定为拉流失败" <<std::endl;
        throw std::runtime_error("拉流失败");
    }
}

OpencvPuller &OpencvPuller::operator>>(cv::Mat &frame)
{
    (this->*_getFrameFn)(frame);
    frame = frame.clone();  // copy一份出来，切断数据关联，防止他在搞事的时候，这个类里给数据改了
    ++_frameID;
    return *this;
}

std::shared_ptr<cv::Mat> OpencvPuller::GetFrame()
{
    cv::Mat frame;
    *this >> frame;
    return std::make_shared<cv::Mat>(frame);
}

bool OpencvPuller::InitFFmpegBackend(const std::string &source)
{
    try{
        _ffStreamer.reset(new FFmpegStreamer(source, true));
        _ffDecoder.reset(new FFmpegDecoder(*_ffStreamer));
    }
    catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
        return false;
    }

    _w = _ffStreamer->GetWidth();
    _h = _ffStreamer->GetHeight();
    _fps = _ffStreamer->GetFPS();
    return true;
}

void OpencvPuller::FFmpegParseFrame(cv::Mat &frame)
{
    while(true){
        auto pkt = _ffStreamer->Parse();
        if(pkt){
            auto data = _ffDecoder->Decode(pkt);
            if(data){
                frame = std::move(cv::Mat(_h, _w, CV_8UC3, data));
                break;
            }
        }
    }
}

bool OpencvPuller::InitOpencvBackend(const std::string &source)
{
    if(std::all_of(source.begin(), source.end(), [](const auto &ch){return isdigit(ch);}))
        _cap.reset(new cv::VideoCapture(std::stoi(source)));
    else
        _cap.reset(new cv::VideoCapture(source));
    if(!_cap->isOpened())
        return false;
    _w = static_cast<uint>(_cap->get(cv::CAP_PROP_FRAME_WIDTH));
    _h = static_cast<uint>(_cap->get(cv::CAP_PROP_FRAME_HEIGHT));
    _fps = _cap->get(cv::CAP_PROP_FPS);
    return true;
}

void OpencvPuller::OpencvParseFrame(cv::Mat &frame)
{
    *_cap >> frame;
}