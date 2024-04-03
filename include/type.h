#pragma once

#include <string>
#include <vector>

// 可通过{x,y}赋值
struct Point{
    int x;
    int y;
};

// 可通过{x,y,w,h}赋值
struct Box{
    // 左上角坐标
    int x;
    int y;
    // 长宽
    uint w;
    uint h;
};

struct Object {
    Box box;  // 位置
    double score;  // 置信度
    size_t classIndex;  // 类别ID
    std::string className;  // 类别名称
};

using ResultType = std::vector<Object>;