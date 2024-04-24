#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include "stream/decoder/soft.h"
#include "stream/decoder/decoder.hpp"
#include "stream/puller/opencv.h"
#include "stream/pusher/ffmpeg.h"
#include "stream/pusher/packet.h"
#include "stream/puller/packet.h"
#include "stream/puller/puller.hpp"
#include "stream/puller/dummy.hpp"
#include "stream/pusher/pusher.hpp"
#include "detect/yolov7.h"
#include "detect/detector.hpp"
#include "draw/opencv.h"
#include "draw/drawer.hpp"
#include "alert/alert.h"
#include "alert/alerter.hpp"
#include "queues.hpp"
#include "alert/track/tracker.h"
#include "alert/track/tracking.h"
#include "alert/alert.h"
#include <ulid/ulid.hh>
#include <json/json.h>
#include <httplib.h>
#include <cpp-base64/base64.h>
#include "draw/draw.hpp"
// extern "C" {
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libavutil/opt.h>
// #include <libavutil/time.h>
// }

// template <typename T>
// std::ostream &operator<<(std::ostream &o, const std::vector<T> &v){
//     o << "(" << v.front() 
//       << std::accumulate(v.begin() + 1, v.end(), std::string(), [](std::string &s, const T &val){return s + (", " + std::to_string(val));})
//       << ")";
//     return o;
// }

/**
 * @brief 启动所有任务
 *
 * 遍历给定的任务列表，依次启动每个任务，并等待它们完成。
 *
 * @param runners 任务列表
 */
template<typename... RunnerType>
void StartRunners(RunnerType&... runner)  // 可变模板参数列表真好玩
{
    std::array<Runner*, sizeof...(runner)> runners = {&runner...};
    // 启动所有任务
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Start();});
    // 等待所有任务结束
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Wait();});
}

template<typename DetectorType>
void StartWithDF(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入绘图模式");
    OpencvPuller puller(program.get("input"));
    FFmpegPusher pusher(program.get("output"), puller.GetWidth(), puller.GetHeight(), puller.GetFPS());
    LightAlerter alerter(
        program.get<std::vector<std::string>>("region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );
    OpencvDrawer drawer;

    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Drawer<decltype(drawer), decltype(resultFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, resultFrameSQ, inputSQ, outputSQ);
    Detector<DetectorType, decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);

    StartRunners(tpuller, tpusher, tdetector, tdrawer, talerter);
}

template<typename DetectorType>
void StartWithoutDF(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入无绘图数据包转发模式");
    PacketPuller puller(program.get("input"));
    PacketPusher pusher(puller, program.get("output"));
    SoftDecoder decoder(puller);
    LightAlerter alerter(
        program.get<std::vector<std::string>>("region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );

    TQueue<16, AVPacket> outputPktSQ, decodePktSQ;
    Dispatcher<AVPacket> pktDispatcher(outputPktSQ, decodePktSQ);  // 将数据分别分发到上面两个队列中，以供解码器和推送器分别使用
    SQueue<cv::Mat> frameSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(pktDispatcher)> tpuller(puller, pktDispatcher);
    Pusher<decltype(pusher), decltype(outputPktSQ)> tpusher(pusher, outputPktSQ);
    Decoder<decltype(decoder), decltype(decodePktSQ), decltype(frameSQ)> tdecoder(decoder, decodePktSQ, frameSQ);
    Detector<DetectorType, decltype(frameSQ), decltype(resultFrameSQ)> tdetector(detector, frameSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);

    StartRunners(tpuller, tpusher, tdecoder, tdetector, talerter);
}

int main(int argc, char const *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    argparse::ArgumentParser program("Edge Warning");

    // 模型参数
    program.add_argument("--model").default_value("model.rknn").help("RKNN模型");
    program.add_argument("--anchors_file").default_value("anchors.txt").help("Anchors文件位置，如果你的模型包含Grid操作，则无需该文件");
    program.add_argument("--classes_file").default_value("classes.txt").help("Classes文件位置，目前只支持英文名称");

    // 视频参数
    program.add_argument("--input").required().help("输入视频流");
    program.add_argument("--output").required().help("输出视频流");
    program.add_argument("--disable_draw_video_box").flag().help("关闭视频画面绘制识别框");

    // 检测参数
    program.add_argument("--obj_thresh").default_value(0.65f).scan<'f', float>().help("对象类别阈值，范围为0~1，置信度为该阈值以下的预测框在检测中会被过滤");
    program.add_argument("--nms_thresh").default_value(0.45f).scan<'f', float>().help("NMS的IOU阈值，范围为0~1，该数值是为了过滤过近的预测框，其值越大，保留的预测框越多");
    program.add_argument("--skip_frame").default_value(1u).scan<'u', uint>().help("实现跳帧检测，设置为1表示不跳帧，设置为n表示跳n帧，即每隔n帧进行一次检测");

    // 追踪参数
    program.add_argument("--track_time_threshhold").default_value(1.0f).scan<'f', float>().help("设置追踪时间阈值（秒），每过一次这个时间会确认一次物体是否停留在画面中");
    program.add_argument("--track_enter_percent_threshhold").default_value(0.6f).scan<'f', float>().help("认为物体在画面中的检测比");
    program.add_argument("--track_leave_percent_threshhold").default_value(0.f).help("认为物体不在画面中的检测比（设为0表示只要物体在一次时间阈值中任意出现在画面中一次即可）");
    program.add_argument("--track_sim_threshhold").default_value(0.25f).help("追踪时，判断前后两帧两个物体是否是同一个物体的物体未知相似度，采用带权重的eiou算法，范围为0~1，数值越小，新一帧检测到的物体匹配到的概率越大");

    // 报警参数
    program.add_argument("--alert_collect_url").required().help("报警收集地址");
    program.add_argument("--disable_draw_alert_box").flag().help("关闭报警图像绘制识别框");

    program.add_argument("--region").nargs(argparse::nargs_pattern::any).help("检测区域信息");
    program.add_argument("--alert").nargs(argparse::nargs_pattern::any).help("报警设置信息");

    // // 调试使用
    // program.at("--input").default_value("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    // program.at("--output").default_value("rtmp://192.168.1.173/live/1699323368155858390");
    // program.at("--alert_collect_url").default_value("http://localhost:5000/alert/collect/1699323368155858390");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        spdlog::critical(err.what());
        spdlog::critical(program.help().str());
        return 1;
    }

    Yolov7 detector(program.get("model"));
    detector.SetAnchors(program.get("anchors_file"));
    detector.SetClasses(program.get("classes_file"));
    detector.SetObjThresh(program.get<float>("obj_thresh"));
    detector.SetNMSThresh(program.get<float>("nms_thresh"));

    // 设置目标追踪参数
    Tracker::SetTrackTimeThreshhold(std::chrono::milliseconds(static_cast<uint>(program.get<float>("track_time_threshhold") * 1000)));
    Tracker::SetTrackEnterPercentThreshhold(program.get<float>("track_enter_percent_threshhold"));
    Tracker::SetTrackLeavePercentThreshhold(program.get<float>("track_leave_percent_threshhold"));
    Tracking::SetTrackSimThreshhold(program.get<float>("track_sim_threshhold"));
    // 设置事件触发器参数
    Trigger::SetAlertUrl(program.get("alert_collect_url"));  // 设置报警Url
    Trigger::SetDrawBox(!program.get<bool>("disable_draw_alert_box"));  // 设置是否绘制报警框

    if(program.get<bool>("disable_draw_video_box"))
        StartWithoutDF(detector, program);
    else
        StartWithDF(detector, program);
    return 0; 
}
