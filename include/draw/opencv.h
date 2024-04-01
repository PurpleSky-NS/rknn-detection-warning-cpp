#pragma once

#include <opencv2/opencv.hpp>
#include "type.h"

class OpencvDrawer
{
public:
    OpencvDrawer() = default;
    OpencvDrawer(const OpencvDrawer&)=default;
    OpencvDrawer(OpencvDrawer&&)=default;
    
    //不能直接在原图上绘制
    std::shared_ptr<cv::Mat> DrawFrame(std::shared_ptr<cv::Mat> frame, std::shared_ptr<const ResultType> results);

};
