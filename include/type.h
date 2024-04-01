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
    Box box;
    float score;
    size_t classIndex;
    std::string className;
};

using ResultType = std::vector<Object>;