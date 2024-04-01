#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>
#include "stream/puller/opencv.h"
#include "stream/pusher/opencv.h"
#include "stream/puller/puller.hpp"
#include "stream/pusher/pusher.hpp"
#include "detect/yolov7.h"
#include "detect/detector.hpp"
#include "draw/opencv.h"
#include "draw/drawer.hpp"
// #include "drawer/cxvFont.hpp"
#include "timer.h"
#include "squeue.hpp"
#include <ulid/ulid.hh>
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


// int main(int argc, char const *argv[])
// {
//     auto [a, b] = mt(1, "aaa");
//     std::cout << a << b << std::endl;
//     return 0;
// }

int main(int argc, char const *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
    
    argparse::ArgumentParser program("Edge Warning");

    program.add_argument("--model").default_value("model.rknn").help("RKNN模型");
    program.add_argument("--anchors_file").default_value("anchors.txt").help("Anchors文件位置");
    program.add_argument("--classes_file").default_value("classes.txt").help("Classes文件位置");
    program.add_argument("--obj_thresh").default_value(0.65f).scan<'f', float>().help("类别阈值");

    // program.add_argument("--input").required().help("输入视频流");
    // program.add_argument("--output").required().help("输出视频流");
    program.add_argument("--input").default_value("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1").help("输入视频流");
    program.add_argument("--output").default_value("rtmp://192.168.1.173/live/1699323368155858390").help("输出视频流");

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
    detector.SetObjThresh(program.get<float>("--obj_thresh"));

    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType> resultSQ;

    // 拉流 
    OpencvPuller puller(program.get("input"));
    // 推流
    OpencvPusher pusher(program.get("output"), puller.GetWidth(), puller.GetHeight(), puller.GetFPS());
    // 绘图
    OpencvDrawer drawer;

    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<decltype(detector), decltype(inputSQ), decltype(resultSQ)> tdetector(detector, inputSQ, resultSQ);
    Drawer<decltype(drawer), decltype(resultSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, resultSQ, inputSQ, outputSQ);

    tpuller.Wait();
    tpusher.Wait();
    tdetector.Wait();
    tdrawer.Wait();

    return 0; 
}
