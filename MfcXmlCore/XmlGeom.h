// XmlGeom.h —— XML 通道的几何算法层（跑在 boost 上，不再手搓数学）。
//
// 分工：
//   boost::qvm          4×4 矩阵与向量（旋转/缩放/平移/斜切的合成、点变换、求逆）
//   boost::geometry     二维几何（变换后四边形的包围盒、点是否落在四边形内、矩形求交）
//
// 为什么不自己写：矩阵乘法、"点是否在旋转后的盒子里"这类判断，手写版本错了也不报错，
// 只会表现为"偶尔点不中/偶尔裁掉一角"。boost 是 header-only，third_party\boost186 里就有，
// 编译期直接用，不引入二进制依赖。
#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <windows.h>

#include <boost/qvm/mat.hpp>
#include <boost/qvm/mat_operations.hpp>
#include <boost/qvm/vec.hpp>
#include <boost/qvm/vec_mat_operations.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/box.hpp>

#include "XmlMatrix4.h"

namespace xmlgeom
{
    typedef boost::geometry::model::d2::point_xy<double> GeoPoint;
    typedef boost::geometry::model::box<GeoPoint> GeoBox;
    typedef boost::geometry::model::polygon<GeoPoint, false, false> GeoPoly;   // 不要求顺时针、闭合
    typedef boost::qvm::mat<double, 4, 4> Mat4;
    typedef boost::qvm::vec<double, 4> Vec4;

    // XmlMatrix4（列向量约定，POD） → boost::qvm 的 4×4（同样按列向量用，直接逐元素搬运）
    inline Mat4 ToQvm(const xmlgeo::XmlMatrix4& m)
    {
        Mat4 r;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                r.a[row][col] = static_cast<double>(m.m[row * 4 + col]);
        return r;
    }

    inline xmlgeo::XmlMatrix4 FromQvm(const Mat4& q)
    {
        xmlgeo::XmlMatrix4 r;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                r.m[row * 4 + col] = static_cast<float>(q.a[row][col]);
        return r;
    }

    // 两个 4×4 相乘（与 XML 侧同一约定：a·b 表示先 b 后 a）
    inline xmlgeo::XmlMatrix4 Compose(const xmlgeo::XmlMatrix4& a, const xmlgeo::XmlMatrix4& b)
    {
        Mat4 qa = ToQvm(a);
        Mat4 qb = ToQvm(b);
        return FromQvm(qa * qb);
    }

    // 元素盒子（矩形）+ 变换 → 变换后的四边形（按 左下/右下/右上/左上 顺序）
    inline GeoPoly QuadOf(const RECT& box, const xmlgeo::XmlMatrix4& m)
    {
        const double xs[4] = { (double)box.left, (double)box.right, (double)box.right, (double)box.left };
        const double ys[4] = { (double)box.top, (double)box.top, (double)box.bottom, (double)box.bottom };
        GeoPoly poly;
        for (int k = 0; k < 4; ++k)
        {
            float ox = 0.0f, oy = 0.0f;
            m.TransformPoint(static_cast<float>(xs[k]), static_cast<float>(ys[k]), 0.0f, ox, oy);
            poly.outer().push_back(GeoPoint(ox, oy));
        }
        boost::geometry::correct(poly);
        return poly;
    }

    // 变换后四边形的轴对齐包围盒 —— 绘制裁剪、脏矩形、命中粗筛都用它。
    // 手写版要自己遍历四角取 min/max，用 boost 一行 envelope。
    inline GeoBox EnvelopeOf(const GeoPoly& poly)
    {
        GeoBox box;
        boost::geometry::envelope(poly, box);
        return box;
    }

    inline RECT ToRect(const GeoBox& box)
    {
        return RECT{ (LONG)std::floor(box.min_corner().x()),
                     (LONG)std::floor(box.min_corner().y()),
                     (LONG)std::ceil(box.max_corner().x()),
                     (LONG)std::ceil(box.max_corner().y()) };
    }

    // 点是否落在变换后的盒子内（HTML 命中测试的等价语义：transform 之后仍按几何外形判定）
    inline bool Contains(const GeoPoly& poly, double x, double y)
    {
        return boost::geometry::covered_by(GeoPoint(x, y), poly);
    }

    // 两个矩形求交（boost 里 box 的交集是标准算法，避免各处手写 IntersectRect 时漏判空集）
    inline bool Intersect(const RECT& a, const RECT& b, RECT& out)
    {
        GeoBox ba{ GeoPoint((double)a.left, (double)a.top), GeoPoint((double)a.right, (double)a.bottom) };
        GeoBox bb{ GeoPoint((double)b.left, (double)b.top), GeoPoint((double)b.right, (double)b.bottom) };
        GeoBox bi;
        if (!boost::geometry::intersection(ba, bb, bi))
            return false;
        out = ToRect(bi);
        return true;
    }
}
