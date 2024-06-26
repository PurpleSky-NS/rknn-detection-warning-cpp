#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

// 可通过{x,y}赋值
// struct Point{
//     int x;
//     int y;
// };
using Point = cv::Point;

// 可通过{x,y,w,h}赋值
struct Box{
    // 左上角坐标
    int x;
    int y;
    // 长宽
    uint w;
    uint h;
    Point GetPos()const;
};

struct Object {
    Box box;  // 位置
    double score;  // 置信度
    size_t classIndex;  // 类别ID
    std::string className;  // 类别名称
};

using ResultType = std::vector<Object>;

inline Point Box::GetPos()const
{
    return {x + static_cast<int>(w) / 2, y + static_cast<int>(h) / 2};
}