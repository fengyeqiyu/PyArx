#pragma once

#include "dwg_reader/ge/box3d.h"
#include "dwg_reader/ge/point3d.h"

namespace dwr {

/// 曲线/曲面几何基类预留（蓝本 AcGeCurve3d / AcGeEntity3d）。
/// 具体派生：Line / LineSeg / Ray / Arc / Ellipse / Spline / Polyline，
/// 随实体几何层 #24/#20 逐步落地；接口约定：包围盒、最近点、交点、参数取点。
class Curve3d {
public:
    virtual ~Curve3d() = default;

    /// 包围盒（待实体层补全实现）。
    virtual Box3d boundingBox() const = 0;
};

}  // namespace dwr
