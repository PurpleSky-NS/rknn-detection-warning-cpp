#pragma once

#include <opencv2/opencv.hpp>
#include "type.h"

void DrawObject(cv::Mat &image, const Object &obj, bool drawName=false);

inline void DrawObject(cv::Mat &image, const Object &obj, bool drawName)
{
    cv::rectangle(image, {obj.box.x, obj.box.y}, {obj.box.x + (int)obj.box.w, obj.box.y + (int)obj.box.h}, {247, 200, 125}, 3);
    if(drawName){
        std::stringstream ss;
        ss << std::setprecision(2) << obj.className << ": " << obj.score;
        auto text = ss.str();
        auto textSize = cv::getTextSize(text, cv::FONT_HERSHEY_COMPLEX, 0.75, 1, nullptr);
        auto y = obj.box.y - textSize.height;
        cv::putText(image, ss.str(), {obj.box.x, y < 0 ? static_cast<int>(obj.box.h) + 1 : y},
            cv::FONT_HERSHEY_COMPLEX, 0.75, {247, 200, 125}, 2);
    }
}
