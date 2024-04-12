#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
// #include <random>
#include "stream/puller/opencv.h"
#include "stream/pusher/opencv.h"
#include "stream/puller/puller.hpp"
#include "stream/puller/dummy.hpp"
#include "stream/pusher/pusher.hpp"
#include "stream/pusher/cvshow.hpp"
#include "detect/yolov7.h"
#include "detect/detector.hpp"
#include "draw/opencv.h"
#include "draw/drawer.hpp"
#include "alert/alert.h"
#include "alert/alerter.hpp"
// #include "drawer/cxvFont.hpp"
#include "timer.h"
#include "squeue.hpp"
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

template<typename PullerType, typename PusherType, typename DetectorType, typename DrawerType, typename AlerterType, typename FrameType=cv::Mat>
void StartRunnerWithDrawer(PullerType &puller, PusherType &pusher, DetectorType &detector, DrawerType &drawer, AlerterType &alerter)
{
    SQueue<FrameType> inputSQ, outputSQ;
    SQueue<ResultType, FrameType> resultFrameSQ;
    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<decltype(detector), decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ);
    Drawer<decltype(drawer), decltype(resultFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, resultFrameSQ, inputSQ, outputSQ);
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);
    std::vector<Runner*> runners{&tpuller, &tpusher, &tdetector, &tdrawer, &talerter};
    // std::vector<Runner*> runners{&tpuller, &tdetector, &tdrawer, &talerter};
    // 启动所有任务
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Start();});
    // 等待所有任务结束
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Wait();});
}

template<typename PullerType, typename PusherType, typename DetectorType, typename AlerterType, typename FrameType=cv::Mat>
void StartRunnerWithoutDrawer(PullerType &puller, PusherType &pusher, DetectorType &detector, AlerterType &alerter)
{
    SQueue<FrameType> frameSQ;
    SQueue<ResultType, FrameType> resultFrameSQ;
    Puller<decltype(puller), decltype(frameSQ)> tpuller(puller, frameSQ);
    Pusher<decltype(pusher), decltype(frameSQ)> tpusher(pusher, frameSQ);
    Detector<decltype(detector), decltype(frameSQ), decltype(resultFrameSQ)> tdetector(detector, frameSQ, resultFrameSQ);
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);
    std::vector<Runner*> runners{&tpuller, &tpusher, &tdetector, &talerter};
    // 启动所有任务
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Start();});
    // 等待所有任务结束
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Wait();});
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
    program.add_argument("--obj_thresh").default_value(0.65f).scan<'f', float>().help("对象类别阈值，范围为0~1，置信度为该阈值以下的预测框在检测中会被过滤");
    program.add_argument("--nms_thresh").default_value(0.45f).scan<'f', float>().help("NMS的IOU阈值，范围为0~1，该数值是为了过滤过近的预测框，其值越大，保留的预测框越多");
    
    // 视频参数
    program.add_argument("--input").required().help("输入视频流");
    program.add_argument("--output").required().help("输出视频流");
    program.add_argument("--disable_push_video").flag().help("禁用视频流推流");
    program.add_argument("--resolution").default_value("").help("转播的分辨率(宽*高)，如640*480");
    program.add_argument("--disable_draw_video_box").flag().help("关闭视频画面绘制识别框");

    // 追踪参数
    program.add_argument("--track_time_threshhold").default_value(1.0f).scan<'f', float>().help("设置追踪时间阈值（秒），每过一次这个时间会确认一次物体是否停留在画面中");
    program.add_argument("--track_enter_percent_threshhold").default_value(0.6f).scan<'f', float>().help("认为物体在画面中的检测比");
    program.add_argument("--track_leave_percent_threshhold").default_value(0.f).help("认为物体不在画面中的检测比（设为0表示只要物体在一次时间阈值中任意出现在画面中一次即可）");

    // 报警参数
    program.add_argument("--alert_collect_url").required().help("报警收集地址");
    program.add_argument("--disable_draw_alert_box").flag().help("关闭报警图像绘制识别框");

    program.add_argument("--ai_region").nargs(argparse::nargs_pattern::any).help("检测区域信息");
    program.add_argument("--alert").nargs(argparse::nargs_pattern::any).help("报警设置信息");

    // 调试使用
    program.at("--input").default_value("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    program.at("--output").default_value("rtmp://192.168.1.173/live/1699323368155858390");
    program.at("--alert_collect_url").default_value("http://localhost:5000/alert/collect/1699323368155858390");

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

    // 拉流 
    OpencvPuller puller(program.get("input"), program.get("resolution"));
    // DummyPuller puller("1.png");
    // 推流
    
    std::unique_ptr<OpencvPusher> ppusher;
    
    if(!program.get<bool>("disable_push_video"))
        ppusher = std::make_unique<OpencvPusher>(program.get("output"), puller.GetWidth(), puller.GetHeight(), puller.GetFPS());

    // CvShowPusher pusher;
    // 报警
    LightAlerter alerter(
        program.get<std::vector<std::string>>("ai_region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );
    // 设置目标追踪参数
    Tracker::SetTrackTimeThreshhold(std::chrono::milliseconds(static_cast<uint>(program.get<float>("track_time_threshhold") * 1000)));
    Tracker::SetTrackEnterPercentThreshhold(program.get<float>("track_enter_percent_threshhold"));
    Tracker::SetTrackLeavePercentThreshhold(program.get<float>("track_leave_percent_threshhold"));
    // 设置事件触发器参数
    Trigger::SetAlertUrl(program.get("alert_collect_url"));  // 设置报警Url
    Trigger::SetDrawBox(!program.get<bool>("disable_draw_alert_box"));  // 设置是否绘制报警框

    std::unique_ptr<OpencvDrawer> pdrawer;
    if(!program.get<bool>("disable_draw_video_box"))
        pdrawer = std::make_unique<std::remove_reference<decltype(*pdrawer)>::type>();

    // 下面开启多线程进行不同的任务
    // 声明数据队列
    std::unique_ptr<SQueue<cv::Mat>> inputSQ, outputSQ;
    std::unique_ptr<SQueue<ResultType, cv::Mat>> resultFrameSQ;
    // 声明任务线程对象
    std::unique_ptr<Puller<decltype(puller), decltype(inputSQ)::element_type>> tpuller;
    std::unique_ptr<Pusher<decltype(ppusher)::element_type, decltype(outputSQ)::element_type>> tpusher;
    std::unique_ptr<Drawer<
        decltype(pdrawer)::element_type, 
        decltype(resultFrameSQ)::element_type, 
        decltype(inputSQ)::element_type, 
        decltype(outputSQ)::element_type
    >> tdrawer;
    std::unique_ptr<Detector<
        decltype(detector), 
        decltype(inputSQ)::element_type, 
        decltype(resultFrameSQ)::element_type
    >> tdetector;
    std::unique_ptr<Alerter<decltype(alerter), decltype(resultFrameSQ)::element_type>> talerter;

    // 初始化数据队列
    inputSQ = std::make_unique<decltype(inputSQ)::element_type>();
    outputSQ = std::make_unique<decltype(outputSQ)::element_type>();
    resultFrameSQ = std::make_unique<decltype(resultFrameSQ)::element_type>();

    // 拉流线程
    tpuller = std::make_unique<decltype(tpuller)::element_type>(puller, *inputSQ);
    // 推流线程
    if(ppusher){
        // 若开启推流
        if(pdrawer){
            // 若开启绘图，则将绘图流程连接上
            outputSQ = std::make_unique<decltype(outputSQ)::element_type>();
            tdrawer = std::make_unique<decltype(tdrawer)::element_type>(*pdrawer, *resultFrameSQ, *inputSQ, *outputSQ);
            tpusher = std::make_unique<decltype(tpusher)::element_type>(*ppusher, *outputSQ);
        }
        else
            tpusher = std::make_unique<decltype(tpusher)::element_type>(*ppusher, *inputSQ);
    }
    // 检测线程
    tdetector = std::make_unique<decltype(tdetector)::element_type>(detector, *inputSQ, *resultFrameSQ);
    // 报警线程
    talerter = std::make_unique<decltype(talerter)::element_type>(alerter, *resultFrameSQ);

    // 启动任务
    std::vector<Runner*> runners{tpuller.get(), tdetector.get(), talerter.get()};
    if(tpusher)
        runners.push_back(tpusher.get());
    if(tdrawer)
        runners.push_back(tdrawer.get());

    // 启动所有任务
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Start();});
    // 等待所有任务结束
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Wait();});

    return 0; 
}
