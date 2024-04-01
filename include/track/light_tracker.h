#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
#include "track/object.h"


class Tracker
{
public:
    Tracker() = default;
    Tracker(const Tracker &) = default;
    Tracker(Tracker &&) = default;

private:
    TrackerSet _trackers;
    TrackerSet _existObjs;
    std::function<void(const ObjectTracker&, const TrackerSet&, )>
}