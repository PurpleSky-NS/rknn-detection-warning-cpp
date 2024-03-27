#pragma once

#include <string>
#include <vector>

struct Object {
    // 左上角坐标
    int x;
    int y;
    // 长宽
    uint w;
    uint h;
    float score;
    size_t classIndex;
    std::string className;
};

using ResultType = std::vector<Object>;