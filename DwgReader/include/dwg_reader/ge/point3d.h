#pragma once

#include <cmath>

#include "dwg_reader/ge/vector3d.h"

namespace dwr {

/// 三维点（蓝本 AcGePoint3d，纯自研值类型）。
struct Point3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Point3d() = default;
    Point3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    double distanceTo(const Point3d& o) const {
        return std::sqrt((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y) +
                         (z - o.z) * (z - o.z));
    }

    Vector3d operator-(const Point3d& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Point3d operator+(const Vector3d& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Point3d operator-(const Vector3d& v) const { return {x - v.x, y - v.y, z - v.z}; }
};

/// 过渡别名：旧代码 Vec3 即 Point3d，逐步替换。
using Vec3 = Point3d;

}  // namespace dwr
