#include "stream/pusher/packet.h"
#include <spdlog/spdlog.h>

PacketPusher::PacketPusher(const PacketPuller &pktPuller, const std::string &output):
    _inputFmtCtx(pktPuller._fmtCtx)
{
    char errBuf[1024];
    if (auto ret = avformat_alloc_output_context2(&_outputFmtCtx, NULL, "flv", output.c_str()); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流上下文构造失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    //申请输出AVStream(rtmp)
    const AVCodec* outCodec = avcodec_find_decoder(_inputFmtCtx->streams[pktPuller._videoStreamIndex]->codecpar->codec_id);
    auto pOutputAVStream = avformat_new_stream(_outputFmtCtx, outCodec);
    if (!pOutputAVStream){
        spdlog::critical("输出流构造失败");
        throw std::runtime_error("推流失败");
    }

    //stream信息复制
    AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);
    if (auto ret = avcodec_parameters_to_context(outCodecCtx, _inputFmtCtx->streams[pktPuller._videoStreamIndex]->codecpar); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流信息复制失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    if (auto ret = avcodec_parameters_from_context(pOutputAVStream->codecpar, outCodecCtx); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("输出流信息复制失败: {}", errBuf);
        throw std::runtime_error("推流失败");
    }

    avcodec_free_context(&outCodecCtx);

    if (!(_outputFmtCtx->flags & AVFMT_NOFILE)) {
        if (auto ret = avio_open2(&_outputFmtCtx->pb, output.c_str(), AVIO_FLAG_WRITE, NULL, NULL); ret < 0) {
            av_strerror(ret, errBuf, sizeof(errBuf));
            spdlog::critical("输出流打开失败：{}", errBuf);
            throw std::runtime_error("推流失败");
        }
    }

    //写rtmp头信息
    if (auto ret = avformat_write_header(_outputFmtCtx, NULL); ret < 0){
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::critical("写入头部信息失败：{}", errBuf);
        throw std::runtime_error("推流失败");
    }

    //打印输出信息
    av_dump_format(_outputFmtCtx, 0, output.c_str(), 1);
}

PacketPusher::~PacketPusher()
{
    av_write_trailer(_outputFmtCtx);
    avio_close(_outputFmtCtx->pb);
    avformat_free_context(_outputFmtCtx);
}

void PacketPusher::operator()(std::shared_ptr<AVPacket> pkt)
{
    if(_lastDts >= pkt->dts)
        return;
    _lastDts = pkt->dts;

    auto inputStream = _inputFmtCtx->streams[pkt->stream_index], outputStream = _outputFmtCtx->streams[0];
    
    AVPacket nPkt;  // 这里得复制一下，因为下面那个写入的函数会改变pkt，导致那边的解码失败
    av_packet_ref(&nPkt, pkt.get());
    nPkt.pts = av_rescale_q_rnd(nPkt.pts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    nPkt.dts = av_rescale_q_rnd(nPkt.dts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    nPkt.duration = av_rescale_q(nPkt.duration, inputStream->time_base, outputStream->time_base);
    nPkt.stream_index = 0;
    if (auto ret = av_interleaved_write_frame(_outputFmtCtx, &nPkt); ret < 0) {
        char errBuf[1024];
        av_strerror(ret, errBuf, sizeof(errBuf));
        spdlog::error("写入数据包时出错：{}", errBuf);
        throw std::runtime_error("推流失败");
    }
    av_packet_unref(&nPkt);
}
