#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include "stream/puller/opencv.h"
#include "stream/pusher/opencv.h"
#include "stream/puller/puller.hpp"
#include "stream/pusher/pusher.hpp"
#include "detect/yolov7.h"
#include "detect/detector.hpp"
#include "drawer/opencv.h"
#include "drawer/drawer.hpp"
#include "timer.h"
#include "squeue.hpp"
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

void test()
{
}

int main(int argc, char const *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    Yolov7 detector("best.deploy.i8.rknn");
    // Yolov7 detector("/home/purple/project/nvrpro-airockchip-yolov7-rknn/src/detect/model.rknn");
    detector.SetAnchors("anchors.txt");
    // detector.SetAnchors("/home/purple/project/nvrpro-airockchip-yolov7-rknn/src/detect/anchors.txt");
    detector.SetClasses("classes.txt");
    detector.SetObjThresh(0.65);

    SQueue<cv::Mat> inputSQ, outputSQ;
    SQueue<ResultType> resultSQ;

    // 拉流 
    OpencvPuller puller("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    // 推流
    OpencvPusher pusher(std::string("rtmp://localhost/live/") + argv[1], puller.GetWidth(), puller.GetHeight(), puller.GetFPS());
    // 绘图
    OpencvDrawer drawer;

    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<decltype(detector), decltype(inputSQ), decltype(resultSQ)> tdetector(detector, inputSQ, resultSQ);
    Drawer<decltype(drawer), decltype(resultSQ), decltype(inputSQ), decltype(inputSQ)> tdrawer(drawer, resultSQ, inputSQ, outputSQ);
    

    tpuller.Wait();
    tpusher.Wait();
    tdetector.Wait();
    tdrawer.Wait();

    return 0; 
}
