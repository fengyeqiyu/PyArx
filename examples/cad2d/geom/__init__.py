# -*- coding: utf-8 -*-
"""geom —— 几何运算核心（模块 2）

纯数学，不碰界面、不碰文档，任何图元/交互/渲染都只依赖这里。
坐标一律 CAD 系：x 向右、y 向上、角度逆时针为正、单位 mm。
"""
import math

EPS = 1e-9


# ---------------------------------------------------------------- 点与向量
def add(a, b):
    return (a[0] + b[0], a[1] + b[1])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def scale(a, k):
    return (a[0] * k, a[1] * k)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1]


def cross(a, b):
    return a[0] * b[1] - a[1] * b[0]


def length(a):
    return math.hypot(a[0], a[1])


def unit(a):
    n = length(a)
    return (a[0] / n, a[1] / n) if n > EPS else (0.0, 0.0)


def polar(base, ang_deg, dist):
    """极坐标定点：从 base 按角度（度）走 dist。"""
    r = math.radians(ang_deg)
    return (base[0] + dist * math.cos(r), base[1] + dist * math.sin(r))


def angle_of(a, b):
    """a→b 的方向角，度，归一化到 [0,360)。"""
    return norm_deg(math.degrees(math.atan2(b[1] - a[1], b[0] - a[0])))


def norm_deg(a):
    return a % 360.0


# ---------------------------------------------------------------- 距离
def dist_pt_pt(p, q):
    return math.hypot(p[0] - q[0], p[1] - q[1])


def dist_pt_seg(p, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    if abs(dx) < EPS and abs(dy) < EPS:
        return dist_pt_pt(p, a)
    t = ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / float(dx * dx + dy * dy)
    t = max(0.0, min(1.0, t))
    return math.hypot(p[0] - (a[0] + t * dx), p[1] - (a[1] + t * dy))


def dist_pt_polyline(p, pts, closed=False):
    best = float("inf")
    for i in range(len(pts) - 1):
        best = min(best, dist_pt_seg(p, pts[i], pts[i + 1]))
    if closed and len(pts) > 2:
        best = min(best, dist_pt_seg(p, pts[-1], pts[0]))
    return best


def dist_pt_circle(p, c, r):
    return abs(dist_pt_pt(p, c) - r)


# ---------------------------------------------------------------- 包围盒
def bbox_points(pts):
    if not pts:
        return (0.0, 0.0, 0.0, 0.0)
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    return (min(xs), min(ys), max(xs), max(ys))


def point_in_polygon(p, pts):
    """射线法判断点是否在闭合多边形内（边界上按内部处理）。"""
    if len(pts) < 3:
        return False
    x, y = p
    inside = False
    n = len(pts)
    j = n - 1
    for i in range(n):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if dist_pt_seg(p, (xi, yi), (xj, yj)) <= 1e-6:
            return True
        if ((yi > y) != (yj > y)):
            xint = (xj - xi) * (y - yi) / (yj - yi) + xi
            if x < xint:
                inside = not inside
        j = i
    return inside


def bbox_union(a, b):
    return (min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]))


def bbox_intersect(a, b):
    return not (a[2] < b[0] or a[0] > b[2] or a[3] < b[1] or a[1] > b[3])


def bbox_contains(outer, inner):
    return (outer[0] <= inner[0] and outer[1] <= inner[1]
            and outer[2] >= inner[2] and outer[3] >= inner[3])


# ---------------------------------------------------------------- 圆弧
def arc_points(c, r, a1, a2, max_step_deg=6.0):
    """把圆弧离散成折线点（用于渲染/命中/交点），保证步长不超过 max_step_deg。"""
    sweep = arc_sweep(a1, a2)
    n = max(2, int(math.ceil(sweep / max_step_deg)) + 1)
    out = []
    for i in range(n):
        a = math.radians(a1 + sweep * (float(i) / (n - 1)))
        out.append((c[0] + r * math.cos(a), c[1] + r * math.sin(a)))
    return out


def arc_sweep(a1, a2):
    """a1→a2 逆时针扫过的角度，0..360。"""
    s = norm_deg(a2 - a1)
    return 360.0 if s < EPS else s


def arc_hit(p, c, r, a1, a2, tol):
    if dist_pt_circle(p, c, r) > tol:
        return False
    a = norm_deg(math.degrees(math.atan2(p[1] - c[1], p[0] - c[0])))
    return norm_deg(a - a1) <= arc_sweep(a1, a2) + 1e-6


def arc_bbox(c, r, a1, a2):
    """圆弧包围盒：端点在弧上必算，再补 0/90/180/270 里落在弧内的象限点。"""
    pts = arc_points(c, r, a1, a2)
    quad = []
    for q in (0, 90, 180, 270):
        if norm_deg(q - a1) <= arc_sweep(a1, a2):
            quad.append(polar(c, q, r))
    return bbox_points(pts + quad)


# ---------------------------------------------------------------- 顶点/极值
def circle_bbox(c, r):
    return (c[0] - r, c[1] - r, c[0] + r, c[1] + r)


def rect_norm(x1, y1, x2, y2):
    return (min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))


if __name__ == "__main__":
    # 自检：圆弧包围盒、点线距、极坐标
    assert abs(dist_pt_seg((0, 5), (0, 0), (10, 0)) - 5.0) < 1e-9
    assert abs(dist_pt_seg((-3, 0), (0, 0), (10, 0)) - 3.0) < 1e-9
    b = arc_bbox((0, 0), 100, 0, 90)
    assert abs(b[2] - 100) < 1e-6 and abs(b[3] - 100) < 1e-6, b
    assert abs(arc_sweep(350, 10) - 20.0) < 1e-9
    p = polar((0, 0), 90, 10)
    assert abs(p[0]) < 1e-9 and abs(p[1] - 10) < 1e-9
    print("geom 自检通过")
