#pragma once
// XmlGeometry —— 绘制与交互共用的纯几何库（无 MFC、无 D2D 依赖）
//
// 设计器、Viewer、Python 侧都从这一个地方拿几何：
//   * 圆弧：永远按"单一半径的正圆"算，由外接框与起止角互推圆心/半径
//   * 直线：按 Direction 把外框对角线还原成两端点
//   * 夹点：方框 8 点 / 椭圆 5 点 / 直线与圆弧 2 点
//   * 命中：夹点命中、点是否在框内、点到折线距离
//
// 角度约定（全库统一，屏幕坐标）：0° = 右（3 点方向），90° = 下，180° = 左，270° = 上；
// sweepDeg 为正表示角度递增，即屏幕上的顺时针。

#include "XmlCore.h"

#include <string>
#include <vector>

namespace xmlgeom
{
    struct Point2
    {
        double x = 0.0;
        double y = 0.0;
        Point2() = default;
        Point2(double px, double py) : x(px), y(py) {}
    };

    struct Rect2
    {
        double left = 0.0;
        double top = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        Rect2() = default;
        Rect2(double l, double t, double r, double b) : left(l), top(t), right(r), bottom(b) {}
        double width() const { return right - left; }
        double height() const { return bottom - top; }
        Point2 center() const { return Point2((left + right) / 2.0, (top + bottom) / 2.0); }
    };

    struct Line2
    {
        Point2 a;
        Point2 b;
    };

    // 一段正圆弧：圆心 + 半径 + 起始角 + 扫掠角
    struct Arc2
    {
        double cx = 0.0;
        double cy = 0.0;
        double radius = 1.0;
        double startDeg = 180.0;
        double sweepDeg = 180.0;
        Point2 pointAt(double deg) const;      // 弧上某角度对应的点
        Point2 startPoint() const { return pointAt(startDeg); }
        Point2 endPoint() const   { return pointAt(startDeg + sweepDeg); }
    };

    // 直线走向（对应 XML 的 Direction 属性）
    enum class LineDir
    {
        Down,        // 左上 → 右下（缺省）
        Up,          // 左下 → 右上
        Horizontal,  // 水平中线
        Vertical,    // 垂直中线
    };

    XML_CORE_API LineDir lineDirFromText(const std::wstring& text);
    XML_CORE_API std::wstring lineDirToText(LineDir dir);

    // 角度归一到 [0, 360)
    XML_CORE_API double normalizeDeg(double deg);

    // 弧覆盖的角度区间在 x / y 方向的 cos、sin 极值（"半径倍数"跨度由此得来）
    XML_CORE_API void arcExtremes(double startDeg, double sweepDeg,
                                   double& minCos, double& maxCos,
                                   double& minSin, double& maxSin);

    // 由"外接框 + 起止角"求圆心与单一半径：弧是正圆的一段，且居中内接于该框
    XML_CORE_API Arc2 arcFromBox(const Rect2& box, double startDeg, double sweepDeg);

    // 由"圆心 + 半径 + 角度"求这段弧的外接框（与 arcFromBox 互逆）
    XML_CORE_API Rect2 arcBounds(const Arc2& arc);

    // 弧采样：返回 steps+1 个点，末点即弧终点
    XML_CORE_API void sampleArc(const Arc2& arc, int steps, std::vector<Point2>& out);
    XML_CORE_API std::vector<Point2> sampleArc(const Arc2& arc, int steps);

    // 三次贝塞尔采样（配电规划图的回路连接线走这条）：返回 steps+1 个点
    XML_CORE_API void bezierPoints(const Point2& p0, const Point2& c1, const Point2& c2,
                                    const Point2& p3, int steps, std::vector<Point2>& out);
    // 竖直 S 形连接线的采样：从 (x0,yTop) 到 (x1,yBottom)，两端切线竖直
    XML_CORE_API std::vector<Point2> sCurvePoints(double x0, double yTop, double x1, double yBottom,
                                                   int steps);

    // 直线：按走向把外框还原成实际两端点
    XML_CORE_API Line2 lineFromBox(const Rect2& box, LineDir dir);

    // 夹点 ---------------------------------------------------------------
    // 方框 8 点：左上、上中、右上、左中、右中、左下、下中、右下
    // （顺序沿用设计器既有的方框夹点编号，勿随意调整）
    XML_CORE_API int boxHandles(const Rect2& box, Point2 out[8]);
    // 椭圆 5 点：圆心、上、下、左、右
    XML_CORE_API int ellipseHandles(const Rect2& box, Point2 out[5]);
    // 直线 2 点：起点、终点
    XML_CORE_API int lineHandles(const Rect2& box, LineDir dir, Point2 out[2]);
    // 圆弧 2 点：弧起点、弧终点
    XML_CORE_API int arcHandles(const Rect2& box, double startDeg, double sweepDeg, Point2 out[2]);

    // 工具 ---------------------------------------------------------------
    // 四边就近取整，并保证宽度高度至少为 1
    XML_CORE_API Rect2 roundToInt(const Rect2& r);

    // 夹点命中：返回命中的下标，未命中返回 -1
    XML_CORE_API int hitPoint(const Point2* pts, int count, const Point2& p, double tol);
    XML_CORE_API bool pointInBox(const Rect2& box, const Point2& p);
    // 点到折线的最近距离（选线、判断是否点在弧上）
    XML_CORE_API double distanceToPolyline(const std::vector<Point2>& pts, const Point2& p);

    // 以下算法由 Boost.Geometry 承担（多段线长度、多边形面积、点在多边形内、凸包、折线简化）
    XML_CORE_API double polylineLength(const std::vector<Point2>& pts);
    XML_CORE_API double polygonArea(const std::vector<Point2>& pts);
    XML_CORE_API bool pointInPolygon(const std::vector<Point2>& pts, const Point2& p);
    XML_CORE_API std::vector<Point2> convexHull(const std::vector<Point2>& pts);
    XML_CORE_API void simplifyPolyline(std::vector<Point2>& pts, double tolerance);
}
