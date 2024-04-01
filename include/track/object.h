#pragma once
#include <string>
#include <ulid/ulid.hh>
#include <chrono>
#include "type.h"

class ObjectTracker
{
public:
    std::string id;
    // 左上角坐标
    int x;
    int y;
    // 长宽
    uint w;
    uint h;
    std::string className;
    std::chrono::time_point createTime, currentTime;
    std::

    ObjectTracker(const Object &obj);
    ObjectTracker(const ObjectTracker &) = default;
    ObjectTracker(ObjectTracker &&) = delete;

};

// Tracker可以被组织为：{类别: [{ID1: 物体1}, {ID2: 物体2}...]}
using TrackerSet = std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<ObjectTracker>>>;

inline ObjectTracker::ObjectTracker(const Object &obj): x(obj.x), y(obj.y), w(obj.w), h(obj.h), className(obj.className) 
{
    id = ulid::Marshal(lid::CreateNowRand());
}