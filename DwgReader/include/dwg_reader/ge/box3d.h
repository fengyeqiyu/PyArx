#pragma once

#include "dwg_reader/ge/point3d.h"

namespace dwr {

/// 三维轴向包围盒（蓝本 AcGeBoundBlock3d 的简化轴对齐版本）。
struct Box3d {
    Point3d min;
    Point3d max;
    bool valid = false;

    void addPoint(const Point3d& p) {
        if (!valid) {
            min = max = p;
            valid = true;
            return;
        }
        min.x = p.x < min.x ? p.x : min.x;
        min.y = p.y < min.y ? p.y : min.y;
        min.z = p.z < min.z ? p.z : min.z;
        max.x = p.x > max.x ? p.x : max.x;
        max.y = p.y > max.y ? p.y : max.y;
        max.z = p.z > max.z ? p.z : max.z;
    }
};

}  // namespace dwr
