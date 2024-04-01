#include "draw/opencv.h"
// #include "draw/cxvFont.hpp"

std::shared_ptr<cv::Mat> OpencvDrawer::DrawFrame(std::shared_ptr<cv::Mat> frame, std::shared_ptr<const ResultType> results)
{
    // 画框
    if(results){
        auto image = std::make_shared<cv::Mat>(frame->clone());
            for (auto &obj: *results) {
                cv::rectangle(*image, {obj.box.x, obj.box.y}, {obj.box.x + (int)obj.box.w, obj.box.y + (int)obj.box.h}, {255, 0, 0}, 5);
                cv::putText(*image, obj.className + ": " + std::to_string(obj.score), {obj.box.x, obj.box.y - 10},
                    cv::FONT_HERSHEY_COMPLEX, 0.5, {255, 0, 0});
            }
        return image;
    }
    return frame;
}