#pragma once

#include <opencv2/opencv.hpp>
#include "type.h"

void DrawObject(cv::Mat &image, const Object &obj, bool drawName=false);

inline void DrawObject(cv::Mat &image, const Object &obj, bool drawName)
{
    cv::rectangle(image, {obj.box.x, obj.box.y}, {obj.box.x + (int)obj.box.w, obj.box.y + (int)obj.box.h}, {247, 200, 125}, 3);
    if(drawName)
        cv::putText(image, obj.className + ": " + std::to_string(obj.score), {obj.box.x, obj.box.y - 10},
            cv::FONT_HERSHEY_COMPLEX, 0.5, {247, 200, 125});
}
