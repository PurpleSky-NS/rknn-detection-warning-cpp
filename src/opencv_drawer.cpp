#include "draw/opencv.h"
#include <fmt/core.h>

OpencvDrawer::OpencvDrawer(const std::vector<Region> &regions, size_t w, size_t h)
{
    if(!regions.empty()) {
        std::vector<std::vector<Point>> regionPts;
        for(auto &region: regions)
            if(!region.GetRegion().empty())
                regionPts.push_back(region.GetRegion());
        if(!regionPts.empty()){
            _regionsMask = cv::Mat(h, w, CV_8UC3, {0, 0, 0});
            cv::fillPoly(_regionsMask, regionPts, {0, 165, 255});
        }
    }
}

std::shared_ptr<cv::Mat> OpencvDrawer::DrawFrame(std::shared_ptr<cv::Mat> frame, std::shared_ptr<TrackerWorld> trackingResults)
{
    // 画框
    if(trackingResults){
        auto image = std::make_shared<cv::Mat>(frame->clone());
        if(!_regionsMask.empty())
            cv::addWeighted(*image, 1, _regionsMask, 0.2, 0, *image);
        for (auto &[_, trackers]: *trackingResults) 
            for (auto &[_, tracker]: trackers){
                DrawObject(
                    *image, 
                    tracker->GetObject(), 
                    fmt::format("[{}]{}:{:.2}", tracker->GetID(), tracker->GetObject().className, tracker->GetObject().score),
                    tracker->Exists() ? cv::Scalar{247, 200, 125} : cv::Scalar{0, 165, 255}  // 打出不同的颜色
                );
                cv::polylines(
                    *image, 
                    std::vector<Point>(tracker->GetTrajectory().begin(), tracker->GetTrajectory().end()), 
                    false, 
                    tracker->Exists() ? cv::Scalar{247, 200, 125} : cv::Scalar{0, 165, 255},
                    2
                );
            }
        return image;
    }
    return frame;
}