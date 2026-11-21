#pragma once

#include <cmath>

namespace dwr {

/// 三维向量（蓝本 AcGeVector3d，纯自研值类型，无 ARX 依赖）。
struct Vector3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vector3d() = default;
    Vector3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    double lengthSq() const { return x * x + y * y + z * z; }
    double length() const { return std::sqrt(lengthSq()); }

    double dot(const Vector3d& o) const { return x * o.x + y * o.y + z * o.z; }

    Vector3d cross(const Vector3d& o) const {
        return Vector3d(y * o.z - z * o.y,
                        z * o.x - x * o.z,
                        x * o.y - y * o.x);
    }

    Vector3d operator+(const Vector3d& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3d operator-(const Vector3d& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3d operator*(double s) const { return {x * s, y * s, z * s}; }

    Vector3d normalized() const {
        double l = length();
        return l > 0.0 ? Vector3d(x / l, y / l, z / l) : Vector3d{};
    }
};

}  // namespace dwr
