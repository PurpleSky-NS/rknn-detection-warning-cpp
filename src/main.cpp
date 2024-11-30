#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include "queues.hpp"
#include "summary.hpp"
#include "stream/decoder/packet.h"
#include "stream/decoder/decoder.hpp"
#include "stream/pusher/image.h"
#include "stream/pusher/packet.h"
#include "stream/puller/packet.h"
#include "stream/puller/puller.hpp"
#include "stream/puller/dummy.hpp"
#include "stream/pusher/pusher.hpp"
#include "detect/yolov7.h"
#include "detect/detector.hpp"
#include "draw/image.h"
#include "draw/drawer.hpp"
#include "alert/alert.h"
#include "alert/trigger/trigger.h"
#include "alert/alerter.hpp"
#include "tracking/light/tracking.h"
#include "tracking/tracking.hpp"

cv::Size GetFinalSize(const std::string &resolution, cv::Size def)
{
    if(resolution.empty())
        return def;
    std::vector<int> res;
    if(auto fd = resolution.find('x'); fd != std::string::npos) {
        def.width = stoi(resolution.substr(0, fd));
        def.height = stoi(resolution.substr(fd + 1));
        spdlog::info("分辨率将被转为{}x{}", def.width, def.height);
    }
    return def;
}

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
void StartWithBaseDrawMode(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入基础绘图模式[输入流解码+输出流编码+绘图+检测+追踪+告警]");
    PacketPuller puller(program.get("input"));
    auto size = GetFinalSize(program.get("convert_resolution"), puller.GetSize());
    PacketDecoder decoder(puller, size);
    ImagePusher pusher(program.get("output"), size, puller.GetFPS(), program.get<uint>("pusher_cbr"), program.get<uint>("pusher_thread"), program.get<bool>("encode_with_h265"), program.get<bool>("low_latency"));
    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), size, 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );  // 这个解析需要什么对象的追踪
    LightTracking tracking(alerter.GetAlertClasses());  // 然后传给追踪器
    ImageDrawer drawer(alerter.GetRegions(), size);

    TQueue<16, AVPacket> decodePktTQ;
    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;
    SQueue<TrackerWorld, cv::Mat> trackingFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(decodePktTQ)> tpuller(puller, decodePktTQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Decoder<decltype(decoder), decltype(decodePktTQ), decltype(inputSQ)> tdecoder(decoder, decodePktTQ, inputSQ);
    Detector<DetectorType, decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Tracking<decltype(tracking), decltype(resultFrameSQ), decltype(trackingFrameSQ)> ttracking(tracking, resultFrameSQ, trackingFrameSQ);
    Drawer<decltype(drawer), decltype(trackingFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, trackingFrameSQ, inputSQ, outputSQ);
    Alerter<decltype(alerter), decltype(trackingFrameSQ)> talerter(alerter, trackingFrameSQ);

    StartRunners(tpuller, tpusher, tdecoder, tdetector, ttracking, tdrawer, talerter);
}

template<typename DetectorType>
void StartWithBaseNoDrawMode(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入基础非绘图模式[输入流解码+输出流编码+检测+追踪+告警]");
    PacketPuller puller(program.get("input"));
    auto size = GetFinalSize(program.get("convert_resolution"), puller.GetSize());
    PacketDecoder decoder(puller, size);
    ImagePusher pusher(program.get("output"), size, puller.GetFPS(), program.get<uint>("pusher_cbr"), program.get<uint>("pusher_thread"), program.get<bool>("encode_with_h265"), program.get<bool>("low_latency"));
    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), size, 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );  // 这个解析需要什么对象的追踪
    LightTracking tracking(alerter.GetAlertClasses());  // 然后传给追踪器

    TQueue<16, AVPacket> decodePktTQ;
    SQueue<cv::Mat> frameSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;
    SQueue<TrackerWorld, cv::Mat> trackingFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(decodePktTQ)> tpuller(puller, decodePktTQ);
    Pusher<decltype(pusher), decltype(frameSQ)> tpusher(pusher, frameSQ);
    Decoder<decltype(decoder), decltype(decodePktTQ), decltype(frameSQ)> tdecoder(decoder, decodePktTQ, frameSQ);
    Detector<DetectorType, decltype(frameSQ), decltype(resultFrameSQ)> tdetector(detector, frameSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Tracking<decltype(tracking), decltype(resultFrameSQ), decltype(trackingFrameSQ)> ttracking(tracking, resultFrameSQ, trackingFrameSQ);
    Alerter<decltype(alerter), decltype(trackingFrameSQ)> talerter(alerter, trackingFrameSQ);

    StartRunners(tpuller, tpusher, tdecoder, tdetector, ttracking, talerter);
}

template<typename DetectorType>
void StartWithForwardMode(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入数据包转发模式[输入流解码+数据包转发+检测+追踪+告警]");
    PacketPuller puller(program.get("input"));
    PacketPusher pusher(puller, program.get("output"));
    auto size = GetFinalSize(program.get("convert_resolution"), puller.GetSize());
    PacketDecoder decoder(puller, size);
    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), size, 
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

template<typename DetectorType>
void StartWithServiceMode(DetectorType &detector, const argparse::ArgumentParser &program)
{
    spdlog::info("程序进入后台服务模式[输入流解码+检测+追踪+告警]");
    PacketPuller puller(program.get("input"));
    auto size = GetFinalSize(program.get("convert_resolution"), puller.GetSize());
    PacketDecoder decoder(puller, size);
    TriggerAlerter alerter(
        program.get<std::vector<std::string>>("region"), size, 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );  // 这个解析需要什么对象的追踪
    LightTracking tracking(alerter.GetAlertClasses());  // 然后传给追踪器

    TQueue<16, AVPacket> decodePktTQ;
    SQueue<cv::Mat> frameSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;
    SQueue<TrackerWorld, cv::Mat> trackingFrameSQ;

    // 声明任务线程对象
    Puller<decltype(puller), decltype(decodePktTQ)> tpuller(puller, decodePktTQ);
    Decoder<decltype(decoder), decltype(decodePktTQ), decltype(frameSQ)> tdecoder(decoder, decodePktTQ, frameSQ);
    Detector<DetectorType, decltype(frameSQ), decltype(resultFrameSQ)> tdetector(detector, frameSQ, resultFrameSQ, program.get<uint>("skip_frame"));
    Tracking<decltype(tracking), decltype(resultFrameSQ), decltype(trackingFrameSQ)> ttracking(tracking, resultFrameSQ, trackingFrameSQ);
    Alerter<decltype(alerter), decltype(trackingFrameSQ)> talerter(alerter, trackingFrameSQ);

    StartRunners(tpuller, tdecoder, tdetector, ttracking, talerter);
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

    // 推流选项
    program.add_argument("--disable_pusher").flag().help("禁用推流，程序以后台服务模式运行，当启用该模式后，所有推流选项均无效");
    program.add_argument("--convert_resolution").default_value("").help("输出分辨率，格式为[宽度x高度]，例如1920x1080，在解码时就会将图像缩放，在后续处理时都会使用该分辨率");
    program.add_argument("--pusher_cbr").default_value(600u).scan<'u', uint>().help("推流时的码率，默认600KBps，以KB为单位，如600表示码率为600KBps，码率越高画面越清晰，但会增加网络带宽与推流开销");
    program.add_argument("--pusher_thread").default_value(1u).scan<'u', uint>().help("推流线程，默认为1，线程越多，推流速度越快，当线程较少时可能会导致推流画面出现突然跳帧，但增加线程提高程序开销");
    program.add_argument("--encode_with_h265").flag().help("编码时使用h265编码器，但会显著增加编码开销，否则默认使用h264编码");
    program.add_argument("--low_latency").flag().help("开启低延迟模式，会显著降低直播画面延迟，但会增大CPU开销并降低帧率");

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
    program.add_argument("--enable_draw_video_box").flag().help("启用视频画面绘制识别框");
    program.add_argument("--debug_log").flag().help("开启调试日志");

    try {
        program.parse_known_args(argc, argv);
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

    fpsSummary.Start();
    if(program.get<bool>("disable_pusher")){
        StartWithServiceMode(detector, program);
    }
    else{
        if(program.get<bool>("enable_draw_video_box"))
            StartWithBaseDrawMode(detector, program);
        else{
            if(auto source = program.get("input"); std::all_of(source.begin(), source.end(), [](const auto &ch){return isdigit(ch);}))
                StartWithBaseNoDrawMode(detector, program);  // rawvideo和flv容器格式不兼容，USB摄像头进入基础无绘图模式
            else
                StartWithForwardMode(detector, program);  // 视频流直接进入数据包转发模式减少开销
        }
    }
    return 0; 
}
