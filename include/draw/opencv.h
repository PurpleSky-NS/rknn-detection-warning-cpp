#pragma once
#include "draw/draw.hpp"
#include "tracking/tracker.hpp"
#include "alert/region.h"

class OpencvDrawer
{
public:
    OpencvDrawer() = default;
    OpencvDrawer(const std::vector<Region> &regions, size_t w, size_t h);
    OpencvDrawer(const OpencvDrawer&)=default;
    OpencvDrawer(OpencvDrawer&&)=default;
    
    //不能直接在原图上绘制
    std::shared_ptr<cv::Mat> DrawFrame(std::shared_ptr<cv::Mat> frame, std::shared_ptr<TrackerWorld> trackingResults);

private:
    cv::Mat _regionsMask;
};
