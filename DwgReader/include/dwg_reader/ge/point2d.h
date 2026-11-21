#pragma once

#include "dwg_reader/ge/vector2d.h"

namespace dwr {

/// 二维点（蓝本 AcGePoint2d）。
struct Point2d {
    double x = 0.0;
    double y = 0.0;

    Point2d() = default;
    Point2d(double x_, double y_) : x(x_), y(y_) {}

    Vector2d operator-(const Point2d& o) const { return {x - o.x, y - o.y}; }
};

}  // namespace dwr
