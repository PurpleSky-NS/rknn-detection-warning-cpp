#include "stream/pusher/opencv.h"

#include <sstream>

OpencvPusher::OpencvPusher(const std::string &output, uint w, uint h, double fps): _dataSize(3*w*h)
{
    std::stringstream cmd;
    cmd << "ffmpeg -fflags nobuffer"
        << " -f rawvideo"
        << " -vcodec rawvideo"
        << " -pix_fmt bgr24"
        << " -s " << w << "x" << h
        << " -r " << fps
        << " -i -"
        << " -c:v libx264"
        << " -pix_fmt yuv420p"
        << " -preset ultrafast"
        << " -f flv"
        << " " << output;
    _ffProc = popen(cmd.str().c_str(), "w");
}

OpencvPusher::OpencvPusher(OpencvPusher &&other): _ffProc(other._ffProc), _dataSize(other._dataSize)
{
    other._dataSize = 0;
    other._ffProc = nullptr;
}

OpencvPusher::~OpencvPusher()
{
    fclose(_ffProc);
    _ffProc = nullptr;
}

void OpencvPusher::PushFrame(std::shared_ptr<const cv::Mat> frame)
{
    *this << *frame;
}

OpencvPusher &OpencvPusher::operator<<(const cv::Mat &frame)
{
    fwrite(frame.data, _dataSize, 1, _ffProc);
    return *this;
}
