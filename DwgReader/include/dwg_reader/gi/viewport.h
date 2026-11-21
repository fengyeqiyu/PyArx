#pragma once

#include "dwg_reader/ge/matrix3d.h"

namespace dwr {

/// 视口预留（蓝本 AcGiViewport）：世界→视图变换、裁剪窗口、缩放。
class GiViewport {
public:
    virtual ~GiViewport() = default;

    const Matrix3d& worldToEye() const { return m_worldToEye; }
    void setWorldToEye(const Matrix3d& m) { m_worldToEye = m; }

private:
    Matrix3d m_worldToEye;
};

}  // namespace dwr
