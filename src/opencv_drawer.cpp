#include "drawer/opencv.h"
// #include "drawer/cxvFont.hpp"


void OpencvDrawer::DrawFrame(cv::Mat &frame, const ResultType &results)
{
    // 画框
    for (auto &obj: results) {
        cv::rectangle(frame, {obj.x, obj.y}, {obj.x + (int)obj.w, obj.y + (int)obj.h}, {0, 0, 255}, 1);
        cv::putText(frame, obj.className + ": " + std::to_string(obj.score), {obj.x, obj.y - 10},
            cv::FONT_HERSHEY_COMPLEX, 0.5, {255, 0, 0});
    }
}