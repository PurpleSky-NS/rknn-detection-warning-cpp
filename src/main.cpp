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
#include "stream/pusher/pusher.hpp"
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

// void saveFrame(AVFrame *pFrame, int width, int height, int iFrame)
// {
//     std::cout << "width: " << width<< " " << "height: " << height <<std::endl;
//     std::cout << "lineSize0: " << pFrame->linesize[0]<< " " << "lineSize1:" << pFrame->linesize[1] <<std::endl;
//     cv::Mat image(height, width, CV_8UC3, pFrame->data[0]);
//     cv::imwrite("what.png", image);
// }


// template<typename... Tps>
// std::tuple<Tps...> mt(Tps &&... values)
// {
//     return std::make_tuple(values...);
// }


int main1(int argc, char const *argv[])
{
    OpencvPuller puller("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    auto frame = puller.GetFrame();
    std::vector<uchar> buf;
    cv::imencode(".jpg", *frame, buf);
    auto b64img = base64_encode(buf.data(), buf.size());
    std::cout << b64img << std::endl;
    return 0;
}

int main(int argc, char const *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    argparse::ArgumentParser program("Edge Warning");

    program.add_argument("--model").default_value("model.rknn").help("RKNN模型");
    program.add_argument("--anchors_file").default_value("anchors.txt").help("Anchors文件位置");
    program.add_argument("--classes_file").default_value("classes.txt").help("Classes文件位置");
    program.add_argument("--obj_thresh").default_value(0.65f).scan<'f', float>().help("类别阈值");

    program.add_argument("--input").required().help("输入视频流");
    program.add_argument("--output").required().help("输出视频流");
    program.add_argument("--alert_collect_url").required().help("报警收集地址");

    // 调试使用
    program.at("--input").default_value("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    program.at("--output").default_value("rtmp://192.168.1.173/live/1699323368155858390");
    program.at("--alert_collect_url").default_value("http://localhost:5000/alert/collect/1699323368155858390");

    program.add_argument("--ai_region").nargs(argparse::nargs_pattern::any).help("检测区域信息");
    program.add_argument("--alert").nargs(argparse::nargs_pattern::any).help("报警设置信息");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        spdlog::critical(err.what());
        spdlog::critical(program.help().str());
        return 1;
    }

    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType, cv::Mat> resultFrameSQ;

    Yolov7 detector(program.get("model"));
    detector.SetAnchors(program.get("anchors_file"));
    detector.SetClasses(program.get("classes_file"));
    detector.SetObjThresh(program.get<float>("obj_thresh"));

    // 拉流 
    OpencvPuller puller(program.get("input"));
    // 推流
    OpencvPusher pusher(program.get("output"), puller.GetWidth(), puller.GetHeight(), puller.GetFPS());
    // 绘图
    OpencvDrawer drawer;
    // 报警
    LightAlerter alerter(
        program.get<std::vector<std::string>>("ai_region"), puller.GetWidth(), puller.GetHeight(), 
        program.get<std::vector<std::string>>("alert"), detector.GetClasses()
    );
    // LightAlerter alerter(
    //     {"{\"name\": \"4033600000000050114\", \"region\": [0.01791530944625407, 0.46606948968512485, 0.8778501628664497, 0.46606948968512485, 0.8778501628664497, 0.9533116178067318, 0.01791530944625407, 0.9533116178067318]}"}, puller.GetWidth(), puller.GetHeight(), 
    //     {
    //         "{\"event\": \"人来了\", \"object\": \"person\", \"condition\": \"出现\", \"region\": null, \"args\": null}", 
    //         "{\"event\": \"人走了\", \"object\": \"person\", \"condition\": \"离开\", \"region\": null, \"args\": null}"
    //     }, detector.GetClasses()
    // );
    // 设置报警Url
    Trigger::SetAlertUrl(program.get("alert_collect_url"));

    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<decltype(detector), decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ);
    Drawer<decltype(drawer), decltype(resultFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, resultFrameSQ, inputSQ, outputSQ);
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);

    std::vector<Runner*> runners{&tpuller, &tpusher, &tdetector, &tdrawer, &talerter};
    // 启动所有任务
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Start();});
    // 等待所有任务结束
    std::for_each(runners.begin(), runners.end(), [](auto runner){runner->Wait();});

    return 0; 
}
