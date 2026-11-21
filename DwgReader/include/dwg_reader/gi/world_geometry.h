#pragma once

#include <vector>

#include "dwg_reader/ge/point3d.h"

namespace dwr {

/// 世界几何流预留（蓝本 AcGiWorldGeometry）。
/// Drawable 经此接口把自身几何交给渲染层：线段 / 多段线 / 弧 / 文字 / 填充。
class GiWorldGeometry {
public:
    virtual ~GiWorldGeometry() = default;

    virtual void line(const Point3d& a, const Point3d& b) = 0;
    virtual void polyline(const std::vector<Point3d>& pts) = 0;
};

}  // namespace dwr
