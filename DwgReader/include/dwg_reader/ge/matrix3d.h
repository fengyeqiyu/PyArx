#pragma once

#include "dwg_reader/ge/point3d.h"
#include "dwg_reader/ge/vector3d.h"

namespace dwr {

/// 4x4 齐次变换矩阵（蓝本 AcGeMatrix3d，列优先：m[col*4 + row]）。
/// 本轮仅落存储与单位阵/点变换预留，复合变换与逆矩阵随实体变换需求补全。
struct Matrix3d {
    double m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    static Matrix3d identity() { return Matrix3d{}; }

    void setToIdentity() { *this = Matrix3d{}; }

    /// 变换点（齐次 w=1）。
    Point3d transformPoint(const Point3d& p) const {
        return Point3d(
            m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
            m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
            m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]);
    }

    /// 变换向量（忽略平移列）。
    Vector3d transformVector(const Vector3d& v) const {
        return Vector3d(
            m[0] * v.x + m[4] * v.y + m[8] * v.z,
            m[1] * v.x + m[5] * v.y + m[9] * v.z,
            m[2] * v.x + m[6] * v.y + m[10] * v.z);
    }
};

}  // namespace dwr
