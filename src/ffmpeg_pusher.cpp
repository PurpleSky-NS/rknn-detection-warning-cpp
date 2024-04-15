#include "stream/pusher/ffmpeg.h"

#include <sstream>

FFmpegPusher::FFmpegPusher(const std::string &output, uint w, uint h, double fps): _dataSize(3*w*h)
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

FFmpegPusher::~FFmpegPusher()
{
    fclose(_ffProc);
    _ffProc = nullptr;
}

void FFmpegPusher::Push(std::shared_ptr<const cv::Mat> frame)
{
    *this << *frame;
}

FFmpegPusher &FFmpegPusher::operator<<(const cv::Mat &frame)
{
    fwrite(frame.data, _dataSize, 1, _ffProc);
    return *this;
}
