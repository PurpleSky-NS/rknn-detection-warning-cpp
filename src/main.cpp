#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include "queues.hpp"
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
#include "draw/draw.hpp"
#include "alert/alert.h"
#include "alert/trigger/trigger.h"
#include "alert/alerter.hpp"
#include "tracking/light/tracking.h"
#include "tracking/tracking.hpp"

/**
 * @brief 启动所有任务
 *
 * 遍历给定的任务列表，依次启动每个任务，并等待它们完成。
 *
 * @param runners 任务列表
 */
template<typename... RunnerType>
void StartRunners(RunnerType&... runner)
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

    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );  // 这个解析需要什么对象的追踪
    LightTracking tracking(alerter.GetAlertClasses());  // 然后传给追踪器
    OpencvDrawer drawer(alerter.GetRegions(), puller.GetWidth(), puller.GetHeight());

    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;
    SQueue<TrackerWorld, cv::Mat> trackingFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<DetectorType, decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Tracking<decltype(tracking), decltype(resultFrameSQ), decltype(trackingFrameSQ)> ttracking(tracking, resultFrameSQ, trackingFrameSQ);
    Drawer<decltype(drawer), decltype(trackingFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, trackingFrameSQ, inputSQ, outputSQ);
    Alerter<decltype(alerter), decltype(trackingFrameSQ)> talerter(alerter, trackingFrameSQ);

    StartRunners(tpuller, tpusher, tdetector, ttracking, tdrawer, talerter);
}

template<typename DetectorType>
void StartWithoutDF(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入无绘图数据包转发模式");
    PacketPuller puller(program.get("input"));
    PacketPusher pusher(puller, program.get("output"));
    SoftDecoder decoder(puller);
    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );
    LightTracking tracking(alerter.GetAlertClasses());  // 然后传给追踪器

    TQueue<16, AVPacket> outputPktTQ, decodePktTQ;
    Dispatcher<AVPacket> pktDispatcher(outputPktTQ, decodePktTQ);  // 将数据分别分发到上面两个队列中，以供解码器和推送器分别使用
    SQueue<cv::Mat> frameSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;
    SQueue<TrackerWorld, cv::Mat> trackingFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(pktDispatcher)> tpuller(puller, pktDispatcher);
    Pusher<decltype(pusher), decltype(outputPktTQ)> tpusher(pusher, outputPktTQ);
    Decoder<decltype(decoder), decltype(decodePktTQ), decltype(frameSQ)> tdecoder(decoder, decodePktTQ, frameSQ);
    Detector<DetectorType, decltype(frameSQ), decltype(resultFrameSQ)> tdetector(detector, frameSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Tracking<decltype(tracking), decltype(resultFrameSQ), decltype(trackingFrameSQ)> ttracking(tracking, resultFrameSQ, trackingFrameSQ);
    Alerter<decltype(alerter), decltype(trackingFrameSQ)> talerter(alerter, trackingFrameSQ);

    StartRunners(tpuller, tpusher, tdecoder, tdetector, ttracking, talerter);
}

int main(int argc, char const *argv[])
{
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
    program.add_argument("--obj_thresh").default_value(0.45f).scan<'f', float>().help("对象类别阈值，范围为0~1，置信度为该阈值以下的预测框在检测中会被过滤");
    program.add_argument("--nms_thresh").default_value(0.45f).scan<'f', float>().help("NMS的IOU阈值，范围为0~1，该数值是为了过滤过近的预测框，其值越大，保留的预测框越多");
    program.add_argument("--skip_frame").default_value(1u).scan<'u', uint>().help("实现跳帧检测，设置为1表示不跳帧，设置为n表示跳n帧，即每隔n帧进行一次检测");

    // 追踪参数
    program.add_argument("--track_time_threshhold").default_value(1.0f).scan<'f', float>().help("设置追踪时间阈值（秒），每过一次这个时间会确认一次物体是否停留在画面中");
    program.add_argument("--track_enter_percent_threshhold").default_value(0.6f).scan<'f', float>().help("认为物体在画面中的检测比");
    program.add_argument("--track_leave_percent_threshhold").default_value(0.f).scan<'f', float>().help("认为物体不在画面中的检测比（设为0表示只要物体在一次时间阈值中任意出现在画面中一次即可）");
    program.add_argument("--track_sim_threshhold").default_value(0.5f).scan<'f', float>().help("追踪时，判断前后两帧两个物体是否是同一个物体的物体未知相似度，采用带权重的eiou算法，范围为0~1，数值越小，新一帧检测到的物体匹配到的概率越大");

    // 报警参数
    program.add_argument("--alert_collect_url").required().help("报警收集地址");
    program.add_argument("--disable_draw_alert_box").flag().help("关闭报警图像绘制识别框");
    program.add_argument("--region").nargs(argparse::nargs_pattern::any).help("检测区域信息");
    program.add_argument("--alert").nargs(argparse::nargs_pattern::any).help("报警设置信息");

    // 调试参数
    program.add_argument("--debug_log").flag().help("开启调试日志");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        spdlog::critical(err.what());
        spdlog::critical(program.help().str());
        return 1;
    }

    if(program.get<bool>("debug_log"))
        spdlog::set_level(spdlog::level::debug);
    else
        spdlog::set_level(spdlog::level::info);

    Yolov7 detector(program.get("model"));
    detector.SetAnchors(program.get("anchors_file"));
    detector.SetClasses(program.get("classes_file"));
    detector.SetObjThresh(program.get<float>("obj_thresh"));
    detector.SetNMSThresh(program.get<float>("nms_thresh"));

    // 设置目标追踪参数
    LightTracker::SetTrackTimeThreshhold(std::chrono::milliseconds(static_cast<uint>(program.get<float>("track_time_threshhold") * 1000)));
    LightTracker::SetTrackEnterPercentThreshhold(program.get<float>("track_enter_percent_threshhold"));
    LightTracker::SetTrackLeavePercentThreshhold(program.get<float>("track_leave_percent_threshhold"));
    LightTracking::SetTrackSimThreshhold(program.get<float>("track_sim_threshhold"));
    // 设置事件触发器参数
    Trigger::SetAlertUrl(program.get("alert_collect_url"));  // 设置报警Url
    Trigger::SetDrawBox(!program.get<bool>("disable_draw_alert_box"));  // 设置是否绘制报警框

    if(program.get<bool>("disable_draw_video_box"))
        StartWithoutDF(detector, program);
    else
        StartWithDF(detector, program);
    return 0; 
}
