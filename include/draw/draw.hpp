#pragma once
#include <opencv2/opencv.hpp>
#include "type.h"

inline void DrawObject(cv::Mat &image, const Object &obj, const std::string &label = {}, const cv::Scalar &color={247, 200, 125})
{
    cv::rectangle(image, {obj.box.x, obj.box.y}, {
        obj.box.x + static_cast<int>(obj.box.w), 
        obj.box.y + static_cast<int>(obj.box.h)
    }, color, 3);
    if(!label.empty()){
        auto labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_COMPLEX, 0.75, 1, nullptr);
        auto y = obj.box.y - labelSize.height;
        cv::putText(image, label, {obj.box.x, y < 0 ? static_cast<int>(obj.box.h) + labelSize.height + 1 : y},
            cv::FONT_HERSHEY_COMPLEX, 0.75, color, 2);
    }
}
