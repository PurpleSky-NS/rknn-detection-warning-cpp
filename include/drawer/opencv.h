#pragma once

#include <opencv2/opencv.hpp>
#include "type.h"

class OpencvDrawer
{
public:
    OpencvDrawer() = default;
    OpencvDrawer(const OpencvDrawer&)=default;
    OpencvDrawer(OpencvDrawer&&)=default;
    
    //直接在原图上绘制
    void DrawFrame(cv::Mat &frame, const ResultType &results);

};
