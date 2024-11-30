#pragma once
#include "tracking/tracker.hpp"
#include "alert/region.h"

class ImageDrawer
{
public:
    ImageDrawer() = default;
    ImageDrawer(const std::vector<Region> &regions, const cv::Size &size);
    ImageDrawer(const ImageDrawer&)=default;
    ImageDrawer(ImageDrawer&&)=default;
    
    //不能直接在原图上绘制
    std::shared_ptr<cv::Mat> operator()(std::shared_ptr<cv::Mat> frame, std::shared_ptr<TrackerWorld> trackingResults);

private:
    cv::Mat _regionsMask;
};
