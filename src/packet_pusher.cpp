#include "stream/pusher/packet.h"

#include <spdlog/spdlog.h>


PacketPusher::PacketPusher(const PacketPuller &pktPuller, const std::string &output):
    _inputFmtCtx(pktPuller._fmtCtx)
{
    avformat_alloc_output_context2(&_outputFmtCtx, NULL, "flv", output.c_str());
    if (!_outputFmtCtx)
    {
        spdlog::critical("输出流上下文构造失败");
        throw std::runtime_error("推流失败");
    }

    //申请输出AVStream(rtmp)
    const AVCodec* outCodec = avcodec_find_decoder(_inputFmtCtx->streams[pktPuller._videoStreamIndex]->codecpar->codec_id);
    auto pOutputAVStream = avformat_new_stream(_outputFmtCtx, outCodec);
    if (!pOutputAVStream)
    {
        spdlog::critical("输出流构造失败");
        throw std::runtime_error("推流失败");
    }

    //stream信息复制
    AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);
    if (avcodec_parameters_to_context(outCodecCtx, _inputFmtCtx->streams[pktPuller._videoStreamIndex]->codecpar) < 0)
    {
        spdlog::critical("输出流信息复制失败");
        throw std::runtime_error("推流失败");
    }

    if (avcodec_parameters_from_context(pOutputAVStream->codecpar, outCodecCtx) < 0)
    {
        spdlog::critical("输出流信息复制失败");
        throw std::runtime_error("推流失败");
    }

    avcodec_free_context(&outCodecCtx);

    char errBuf[1024];
    if (!(_outputFmtCtx->flags & AVFMT_NOFILE)) {
        if (auto ret = avio_open2(&_outputFmtCtx->pb, output.c_str(), AVIO_FLAG_WRITE, NULL, NULL); ret < 0) {
            av_strerror(ret, errBuf, sizeof(errBuf));
            spdlog::critical("输出流打开失败：{}", errBuf);
            throw std::runtime_error("推流失败");
        }
    }

    //写rtmp头信息
    if (auto ret = avformat_write_header(_outputFmtCtx, NULL); ret < 0)
    {
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("写入头部信息失败：{}", errBuf);
        throw std::runtime_error("推流失败");
    }

}

PacketPusher::~PacketPusher()
{
    avformat_free_context(_outputFmtCtx);
}

void PacketPusher::Push(std::shared_ptr<AVPacket> pkt)
{
    *this << *pkt;
}

PacketPusher &PacketPusher::operator<<(AVPacket &oriPkt)
{
    if(_lastDts >= oriPkt.dts)
        return *this;
    _lastDts = oriPkt.dts;

    auto inputStream = _inputFmtCtx->streams[oriPkt.stream_index], outputStream = _outputFmtCtx->streams[0];
    
    AVPacket pkt;  // 这里得复制一下，因为下面那个写入的函数会改变pkt，导致那边的解码失败
    av_packet_ref(&pkt, &oriPkt);
    pkt.pts = av_rescale_q_rnd(pkt.pts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, inputStream->time_base, outputStream->time_base);
    pkt.stream_index = 0;
    if (av_interleaved_write_frame(_outputFmtCtx, &pkt) < 0)
        spdlog::error("写入数据包时出错");
    return *this;
}