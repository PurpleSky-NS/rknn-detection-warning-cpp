#pragma once

#include <opencv2/opencv.hpp>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}
#include "stream/puller/packet.h"


/** 方便基于FFmpeg数据包解码用，将数据包软解码成指定格式 */
class   SoftDecoder
{
public:
    SoftDecoder(const PacketPuller &streamer);
    SoftDecoder(const SoftDecoder&)=delete;
    SoftDecoder(SoftDecoder&&)=delete;
    ~SoftDecoder();

    // 解码一帧并返回内置缓存，注意再次调用后会修改内置缓存，如有并发调用该接口的需求请及时将数据复制出来
    std::shared_ptr<cv::Mat> Decode(std::shared_ptr<AVPacket> pkt);

private:
    AVCodecContext *_codecCtx=nullptr;
    AVFrame *_originalFrame=nullptr;
    AVFrame *_decodedFrame=nullptr;
    uint8_t *_imageBuffer=nullptr;
    SwsContext *_swsCtx=nullptr;
};