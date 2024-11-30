#include "draw/image.h"
#include "draw/draw.hpp"
#include <fmt/core.h>

ImageDrawer::ImageDrawer(const std::vector<Region> &regions, const cv::Size &size)
{
    if(!regions.empty()) {
        std::vector<std::vector<Point>> regionPts;
        for(auto &region: regions)
            if(!region.GetRegion().empty())
                regionPts.push_back(region.GetRegion());
        if(!regionPts.empty()){
            _regionsMask = cv::Mat(size.height, size.width, CV_8UC3, {0, 0, 0});
            cv::fillPoly(_regionsMask, regionPts, {0, 165, 255});
        }
    }
}

std::shared_ptr<cv::Mat> ImageDrawer::operator()(std::shared_ptr<cv::Mat> frame, std::shared_ptr<TrackerWorld> trackingResults)
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