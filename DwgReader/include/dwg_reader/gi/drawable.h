#pragma once

#include "dwg_reader/ge/box3d.h"
#include "dwg_reader/gi/world_geometry.h"

namespace dwr {

/// 可绘制对象基类（蓝本 AcGiDrawable）。
/// db 实体在显示阶段实现本接口，向 gi 渲染流输出世界几何；
/// 后期接 gi 视口/裁剪/样式时由渲染层调用 worldDraw。
class Drawable {
public:
    virtual ~Drawable() = default;

    /// 向世界几何流输出自身几何（主线绘制）。
    virtual void worldDraw(GiWorldGeometry& geom) = 0;

    /// 包围盒：视口裁剪与可见性判断用。
    virtual Box3d boundingBox() const = 0;
};

}  // namespace dwr
