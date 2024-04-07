#include "draw/opencv.h"
// #include "draw/cxvFont.hpp"

std::shared_ptr<cv::Mat> OpencvDrawer::DrawFrame(std::shared_ptr<cv::Mat> frame, std::shared_ptr<const ResultType> results)
{
    // 画框
    if(results){
        auto image = std::make_shared<cv::Mat>(frame->clone());
            for (auto &obj: *results) 
                DrawObject(*image, obj, true);
        return image;
    }
    return frame;
}