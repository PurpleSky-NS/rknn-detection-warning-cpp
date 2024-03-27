#pragma once

#include <string>
#include <optional>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

/** 方便基于FFmpeg解析视频流用，这个类处理到数据包那一步 */
class FFmpegStreamer
{
public:
    FFmpegStreamer(const std::string &url, bool dumpFmt=false);
    FFmpegStreamer(const FFmpegStreamer&)=delete;
    FFmpegStreamer(FFmpegStreamer&&)=delete;
    ~FFmpegStreamer();

    uint GetWidth()const;
    uint GetHeight()const;
    double GetFPS()const;

    const AVPacket* Parse();

private:
    friend class FFmpegDecoder;
    uint _videoStreamIndex;
    AVFormatContext *_fmtCtx=nullptr;
    AVPacket *_pkt=nullptr;
};

/** 方便基于FFmpeg数据包解码用，将数据包软解码成指定格式 */
class FFmpegDecoder
{
public:
    FFmpegDecoder(const FFmpegStreamer &streamer, AVPixelFormat decodeFmt=AV_PIX_FMT_BGR24);
    FFmpegDecoder(const FFmpegDecoder&)=delete;
    FFmpegDecoder(FFmpegDecoder&&)=delete;
    ~FFmpegDecoder();

    uint8_t *Decode(const AVPacket *pkt);

private:
    AVCodecContext *_codecCtx=nullptr;
    AVFrame *_originalFrame=nullptr;
    AVFrame *_decodedFrame=nullptr;
    uint8_t *_imageBuffer=nullptr;
    SwsContext *_swsCtx=nullptr;
};