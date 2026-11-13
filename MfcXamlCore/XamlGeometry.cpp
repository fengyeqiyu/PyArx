// XamlGeometry 实现。
// 平面几何算法（点/线/多段线/多边形的距离、包含、相交、面积、凸包、简化）统一由
// Boost.Geometry（header-only，位于 third_party\boost186）承担；本库只保留 XAML 图元
// 特有的参数化圆弧（圆心/半径/起止角）换算——圆周弧不属于 OGC 简单要素模型，
// Boost.Geometry 没有对应图元，这部分由本库按工具库给的点线算法组合而成。
#include "XamlGeometry.h"

#include <algorithm>
#include <cmath>

// ---- Boost.Geometry 接入 ----
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/strategies/strategies.hpp>

namespace xamlgeom
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kDeg2Rad = kPi / 180.0;
        constexpr double kEps = 1e-9;

        // 把本库的 Point2 注册成 Boost.Geometry 的点类型（不额外拷贝，直接复用成员）
        namespace bg = boost::geometry;
    }
}

BOOST_GEOMETRY_REGISTER_POINT_2D(xamlgeom::Point2, double, boost::geometry::cs::cartesian, x, y)

namespace xamlgeom
{
    namespace
    {
        using BgPoint = Point2;                 // 已注册，可直接用于 Boost
        using BgLineString = bg::model::linestring<BgPoint>;
        using BgPolygon = bg::model::polygon<BgPoint, false, false>;   // 屏幕坐标：y 向下，顺时针

        BgLineString toLineString(const std::vector<Point2>& pts)
        {
            BgLineString ls;
            ls.reserve(pts.size());
            for (const Point2& p : pts) ls.push_back(p);
            return ls;
        }

        BgPolygon toPolygon(const std::vector<Point2>& pts)
        {
            BgPolygon poly;
            if (pts.size() < 3) return poly;
            poly.outer().reserve(pts.size() + 1);
            for (const Point2& p : pts) poly.outer().push_back(p);
            poly.outer().push_back(pts.front());    // 闭合
            bg::correct(poly);
            return poly;
        }
    }

    Point2 Arc2::pointAt(double deg) const
    {
        const double rad = deg * kDeg2Rad;
        return Point2(cx + radius * std::cos(rad), cy + radius * std::sin(rad));
    }

    LineDir lineDirFromText(const std::wstring& text)
    {
        if (::_wcsicmp(text.c_str(), L"Up") == 0) return LineDir::Up;
        if (::_wcsicmp(text.c_str(), L"H") == 0) return LineDir::Horizontal;
        if (::_wcsicmp(text.c_str(), L"Horizontal") == 0) return LineDir::Horizontal;
        if (::_wcsicmp(text.c_str(), L"V") == 0) return LineDir::Vertical;
        if (::_wcsicmp(text.c_str(), L"Vertical") == 0) return LineDir::Vertical;
        return LineDir::Down;
    }

    std::wstring lineDirToText(LineDir dir)
    {
        switch (dir)
        {
        case LineDir::Up: return L"Up";
        case LineDir::Horizontal: return L"H";
        case LineDir::Vertical: return L"V";
        default: return L"Down";
        }
    }

    double normalizeDeg(double deg)
    {
        double v = std::fmod(deg, 360.0);
        if (v < 0.0) v += 360.0;
        if (v >= 360.0) v -= 360.0;
        return v;
    }

    void arcExtremes(double startDeg, double sweepDeg,
                     double& minCos, double& maxCos,
                     double& minSin, double& maxSin)
    {
        const double sweep = (std::fabs(sweepDeg) < kEps) ? 180.0 : sweepDeg;
        const double a0 = std::min(startDeg, startDeg + sweep);
        const double a1 = std::max(startDeg, startDeg + sweep);
        const bool full = std::fabs(sweep) >= 360.0 - kEps;

        if (full)
        {
            minCos = minSin = -1.0;
            maxCos = maxSin = 1.0;
            return;
        }

        auto covered = [a0, a1](double deg)
        {
            // deg 的某个 360 平移是否落在 [a0, a1] 内
            double v = deg + std::floor((a0 - deg) / 360.0) * 360.0;
            for (int i = 0; i < 3; ++i)
            {
                if (v >= a0 - kEps && v <= a1 + kEps) return true;
                v += 360.0;
            }
            return false;
        };

        const double r0 = a0 * kDeg2Rad;
        const double r1 = a1 * kDeg2Rad;
        minCos = std::min(std::cos(r0), std::cos(r1));
        maxCos = std::max(std::cos(r0), std::cos(r1));
        minSin = std::min(std::sin(r0), std::sin(r1));
        maxSin = std::max(std::sin(r0), std::sin(r1));

        if (covered(0.0))   maxCos = 1.0;
        if (covered(180.0)) minCos = -1.0;
        if (covered(90.0))  maxSin = 1.0;
        if (covered(270.0)) minSin = -1.0;
    }

    Arc2 arcFromBox(const Rect2& box, double startDeg, double sweepDeg)
    {
        Arc2 arc;
        arc.startDeg = startDeg;
        arc.sweepDeg = sweepDeg;

        double minC = 0, maxC = 0, minS = 0, maxS = 0;
        arcExtremes(startDeg, sweepDeg, minC, maxC, minS, maxS);

        const double cSpan = maxC - minC;      // 弧在 x 方向占的"半径倍数"
        const double sSpan = maxS - minS;      // 弧在 y 方向占的"半径倍数"
        const double w = (box.width() > 2.0) ? box.width() : 2.0;
        const double h = (box.height() > 2.0) ? box.height() : 2.0;

        const double rByX = (cSpan > 1e-6) ? w / cSpan : 1e30;
        const double rByY = (sSpan > 1e-6) ? h / sSpan : 1e30;
        double r = (rByX < rByY) ? rByX : rByY;
        if (r > 1e29) r = std::min(w, h) / 2.0;
        if (r < 1.0) r = 1.0;

        arc.radius = r;
        // 弧居中内接于框：把弧的包围盒摆到框里，多出来的空间两侧均分
        arc.cx = box.left + (-minC) * r + (w - cSpan * r) / 2.0;
        arc.cy = box.top + (-minS) * r + (h - sSpan * r) / 2.0;
        return arc;
    }

    Rect2 arcBounds(const Arc2& arc)
    {
        double minC = 0, maxC = 0, minS = 0, maxS = 0;
        arcExtremes(arc.startDeg, arc.sweepDeg, minC, maxC, minS, maxS);
        Rect2 r;
        r.left = arc.cx + minC * arc.radius;
        r.right = arc.cx + maxC * arc.radius;
        r.top = arc.cy + minS * arc.radius;
        r.bottom = arc.cy + maxS * arc.radius;
        return r;
    }

    void sampleArc(const Arc2& arc, int steps, std::vector<Point2>& out)
    {
        out.clear();
        if (steps < 4) steps = 4;
        out.reserve((size_t)steps + 1);
        const double sweep = (std::fabs(arc.sweepDeg) < kEps) ? 180.0 : arc.sweepDeg;
        for (int i = 0; i <= steps; ++i)
            out.push_back(arc.pointAt(arc.startDeg + sweep * i / steps));
    }

    std::vector<Point2> sampleArc(const Arc2& arc, int steps)
    {
        std::vector<Point2> pts;
        sampleArc(arc, steps, pts);
        return pts;
    }

    void bezierPoints(const Point2& p0, const Point2& c1, const Point2& c2,
                      const Point2& p3, int steps, std::vector<Point2>& out)
    {
        out.clear();
        if (steps < 4) steps = 4;
        out.reserve((size_t)steps + 1);
        for (int i = 0; i <= steps; ++i)
        {
            const double t = (double)i / steps;
            const double u = 1.0 - t;
            const double b0 = u * u * u;
            const double b1 = 3.0 * u * u * t;
            const double b2 = 3.0 * u * t * t;
            const double b3 = t * t * t;
            out.push_back(Point2(p0.x * b0 + c1.x * b1 + c2.x * b2 + p3.x * b3,
                                 p0.y * b0 + c1.y * b1 + c2.y * b2 + p3.y * b3));
        }
    }

    std::vector<Point2> sCurvePoints(double x0, double yTop, double x1, double yBottom, int steps)
    {
        // 竖直 S：两端切线向右下/正下，控制点各取纵向一半，横向留在两端，
        // 于是曲线在 y 方向单调，配合树形布局"同层互不重叠"即可保证不交叉
        Point2 p0(x0, yTop);
        Point2 c1(x0, yTop + (yBottom - yTop) * 0.5);
        Point2 c2(x1, yBottom - (yBottom - yTop) * 0.5);
        Point2 p3(x1, yBottom);
        std::vector<Point2> pts;
        bezierPoints(p0, c1, c2, p3, steps, pts);
        return pts;
    }

    Line2 lineFromBox(const Rect2& box, LineDir dir)
    {
        Line2 line;
        line.a = Point2(box.left, box.top);
        line.b = Point2(box.right, box.bottom);
        switch (dir)
        {
        case LineDir::Up:
            line.a = Point2(box.left, box.bottom);
            line.b = Point2(box.right, box.top);
            break;
        case LineDir::Horizontal:
            line.a = Point2(box.left, (box.top + box.bottom) / 2.0);
            line.b = Point2(box.right, (box.top + box.bottom) / 2.0);
            break;
        case LineDir::Vertical:
            line.a = Point2((box.left + box.right) / 2.0, box.top);
            line.b = Point2((box.left + box.right) / 2.0, box.bottom);
            break;
        default:
            break;
        }
        return line;
    }

    int boxHandles(const Rect2& box, Point2 out[8])
    {
        const double mx = (box.left + box.right) / 2.0;
        const double my = (box.top + box.bottom) / 2.0;
        out[0] = Point2(box.left, box.top);      // 左上
        out[1] = Point2(mx, box.top);            // 上中
        out[2] = Point2(box.right, box.top);     // 右上
        out[3] = Point2(box.left, my);           // 左中
        out[4] = Point2(box.right, my);          // 右中
        out[5] = Point2(box.left, box.bottom);   // 左下
        out[6] = Point2(mx, box.bottom);         // 下中
        out[7] = Point2(box.right, box.bottom);  // 右下
        return 8;
    }

    int ellipseHandles(const Rect2& box, Point2 out[5])
    {
        const double mx = (box.left + box.right) / 2.0;
        const double my = (box.top + box.bottom) / 2.0;
        out[0] = Point2(mx, my);            // 圆心
        out[1] = Point2(mx, box.top);       // 上
        out[2] = Point2(mx, box.bottom);    // 下
        out[3] = Point2(box.left, my);      // 左
        out[4] = Point2(box.right, my);     // 右
        return 5;
    }

    int lineHandles(const Rect2& box, LineDir dir, Point2 out[2])
    {
        const Line2 line = lineFromBox(box, dir);
        out[0] = line.a;
        out[1] = line.b;
        return 2;
    }

    int arcHandles(const Rect2& box, double startDeg, double sweepDeg, Point2 out[2])
    {
        const Arc2 arc = arcFromBox(box, startDeg, sweepDeg);
        out[0] = arc.startPoint();
        out[1] = arc.endPoint();
        return 2;
    }

    Rect2 roundToInt(const Rect2& r)
    {
        Rect2 out;
        out.left = std::floor(r.left + 0.5);
        out.top = std::floor(r.top + 0.5);
        out.right = std::floor(r.right + 0.5);
        out.bottom = std::floor(r.bottom + 0.5);
        if (out.right <= out.left) out.right = out.left + 1.0;
        if (out.bottom <= out.top) out.bottom = out.top + 1.0;
        return out;
    }

    int hitPoint(const Point2* pts, int count, const Point2& p, double tol)
    {
        if (pts == nullptr) return -1;
        for (int i = 0; i < count; ++i)
            if (std::fabs(p.x - pts[i].x) <= tol && std::fabs(p.y - pts[i].y) <= tol)
                return i;
        return -1;
    }

    bool pointInBox(const Rect2& box, const Point2& p)
    {
        bg::model::box<BgPoint> b;
        b.min_corner().x = box.left;
        b.min_corner().y = box.top;
        b.max_corner().x = box.right;
        b.max_corner().y = box.bottom;
        return bg::covered_by(p, b);
    }

    double distanceToPolyline(const std::vector<Point2>& pts, const Point2& p)
    {
        if (pts.empty()) return 1e30;
        if (pts.size() == 1) return bg::distance(pts[0], p);
        return bg::distance(p, toLineString(pts));
    }

    // ---- 下面几项在同一条 Boost.Geometry 通道上扩展，供绘制/命中/后续路径编辑复用 ----

    double polylineLength(const std::vector<Point2>& pts)
    {
        if (pts.size() < 2) return 0.0;
        return bg::length(toLineString(pts));
    }

    double polygonArea(const std::vector<Point2>& pts)
    {
        if (pts.size() < 3) return 0.0;
        return std::fabs(bg::area(toPolygon(pts)));
    }

    bool pointInPolygon(const std::vector<Point2>& pts, const Point2& p)
    {
        if (pts.size() < 3) return false;
        return bg::covered_by(p, toPolygon(pts));
    }

    std::vector<Point2> convexHull(const std::vector<Point2>& pts)
    {
        BgPolygon hull;
        bg::convex_hull(toPolygon(pts.size() >= 3 ? pts : std::vector<Point2>()), hull);
        std::vector<Point2> out;
        if (hull.outer().size() > 1)
        {
            // Boost 返回的环首尾同点，去掉重复的末点
            out.assign(hull.outer().begin(), hull.outer().end() - 1);
        }
        else if (pts.size() == 1)
        {
            out.push_back(pts[0]);
        }
        else if (pts.size() == 2)
        {
            out.push_back(pts[0]);
            out.push_back(pts[1]);
        }
        return out;
    }

    void simplifyPolyline(std::vector<Point2>& pts, double tolerance)
    {
        if (pts.size() < 3) return;
        BgLineString ls = toLineString(pts);
        BgLineString out;
        // 屏幕坐标是投影平面，直接按欧氏距离做 Douglas-Peucker
        bg::simplify(ls, out, tolerance < 0.0 ? 0.0 : tolerance);
        pts.assign(out.begin(), out.end());
    }
}
