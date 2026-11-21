#pragma once

namespace dwr {

/// 二维向量（蓝本 AcGeVector2d）。
struct Vector2d {
    double x = 0.0;
    double y = 0.0;

    Vector2d() = default;
    Vector2d(double x_, double y_) : x(x_), y(y_) {}
};

}  // namespace dwr
