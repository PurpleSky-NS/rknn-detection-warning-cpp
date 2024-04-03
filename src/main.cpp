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
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    LightAlerter alert({
        "{\"name\": \"什么玩意1\", \"region\": [0.1, 0.1, 0.2, 0.2, 0.15, 0.1]}", 
        "{\"name\": \"什么玩意2\", \"region\": [0.1, 0.1, 0.2, 0.2, 0.15, 0.1, 0.0, 0.05]}"
    }, 500, 600, {
        "{\"event\": \"什么玩意要报警1\", \"condition\": \"出现\", \"region\": \"什么玩意1\", \"object\": \"人1\", \"args\": null}",
        "{\"event\": \"什么玩意要报警2\", \"condition\": \"出现\", \"region\": \"什么玩意1\", \"object\": \"人2\", \"args\": null}"
    }, {"人1", "人2", "人3"});
    // Json::Reader reader;
	// Json::Value root;
    // if(!reader.parse(std::string("null"), root)){
    //     throw std::invalid_argument("事件触发器构造失败");
    // }
    // std::cout << root.isNull() << std::endl;
    return 0;
}

int main2(int argc, char const *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    std::default_random_engine e;
    std::uniform_int_distribution<int> pos(-20, 20);
    std::uniform_int_distribution<int> size(0, 5);
    std::bernoulli_distribution b(.5);
    auto du = std::chrono::milliseconds(20);

    Tracking tr({1});
    tr.SetEnterCallback([](auto tracker, auto world, auto image){spdlog::debug("[({}){}] 进入了画面", tracker->GetObject().className, tracker->GetID());});
    tr.SetLeaveCallback([](auto tracker, auto world, auto image){spdlog::debug("[({}){}] 离开了画面", tracker->GetObject().className, tracker->GetID());});
    tr.SetUpdateCallback([](auto world, auto image){spdlog::debug("画面中有[{}]个物体", world.size());});
    // Tracker ot({0, 0, 25, 25, 0.8, 1, "person"});
    for(Object obj={0, 0, 25, 25, 0.5, 1, "人"};;obj.box.x+=pos(e), obj.box.y+=pos(e), obj.box.w += size(e), obj.box.h += size(e)){
        if(b(e))
            tr.Update({obj}, nullptr);
        else
            tr.Update({}, nullptr);
        std::this_thread::sleep_for(du);
    }
    
    // std::unordered_map<size_t, std::unordered_map<std::string, int>> a{{0, {{"00", 1}, {"00", 2}}}, {1, {{"11", 3}, {"11", 4}}}};
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

    // program.add_argument("--input").required().help("输入视频流");
    // program.add_argument("--output").required().help("输出视频流");
    program.add_argument("--input").default_value("rtsp://admin:a123456789@192.168.1.65:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1").help("输入视频流");
    program.add_argument("--output").default_value("rtmp://192.168.1.173/live/1699323368155858390").help("输出视频流");

    program.add_argument("--ai_region").nargs('+').help("检测区域信息");
    program.add_argument("--alert").nargs('+').help("报警设置信息");

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

    Puller<decltype(puller), decltype(inputSQ)> tpuller(puller, inputSQ);
    Pusher<decltype(pusher), decltype(outputSQ)> tpusher(pusher, outputSQ);
    Detector<decltype(detector), decltype(inputSQ), decltype(resultFrameSQ)> tdetector(detector, inputSQ, resultFrameSQ);
    Drawer<decltype(drawer), decltype(resultFrameSQ), decltype(inputSQ), decltype(outputSQ)> tdrawer(drawer, resultFrameSQ, inputSQ, outputSQ);
    Alerter<decltype(alerter), decltype(resultFrameSQ)> talerter(alerter, resultFrameSQ);

    tpuller.Wait();
    tpusher.Wait();
    tdetector.Wait();
    tdrawer.Wait();
    talerter.Wait();

    return 0; 
}
