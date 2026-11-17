# -*- coding: utf-8 -*-
"""ent —— 实体类（模块 5）

一个实体只负责三件事：自己的几何（bbox / 离散点 / 命中 / 夹点）和样式引用。
它不知道屏幕、不知道 SVG、不知道图形是白底还是黑底 —— 那是 render 的事。
这样同一份实体可以渲成 SVG（画布）、PNG（出图）、DXF（交换），互不牵连。

样式约定：color=0 / width=0 / linetype="" 表示"随层"。
"""
import math

import geom
from geom import (arc_bbox, arc_hit, arc_points, arc_sweep, bbox_points,
                  circle_bbox, dist_pt_pt, dist_pt_polyline, dist_pt_seg,
                  point_in_polygon, rect_norm)


class Entity(object):
    kind = "entity"

    def __init__(self, layer="0", color=0, width=0.0, linetype=""):
        self.id = 0
        self.handle = ""          # 图元句柄，保存/引用时用
        self.layer = layer
        self.color = color        # 0 = 随层
        self.width = width        # 0 = 随层
        self.linetype = linetype  # "" = 随层

    # --- 几何，子类实现 ---
    def bbox(self):
        raise NotImplementedError

    def points(self):
        """离散成折线点：渲染线型、命中、与别的实体求交都用它。"""
        raise NotImplementedError

    def hit(self, p, tol):
        return dist_pt_polyline(p, self.points(), self.is_closed()) <= tol

    def is_closed(self):
        return False

    def grips(self):
        return []

    def move_by(self, dx, dy):
        raise NotImplementedError

    def scale_by(self, base, k):
        """绕 base 等比缩放 k（默认不改）。"""

    def rotate_by(self, base, angle_deg):
        """绕 base 旋转 angle_deg 度（默认不改）。"""

    def set_grip(self, index, p):
        """拖动第 index 个夹点到 p（默认整图移动）。"""
        self.move_by(p[0] - self.grips()[index][0],
                     p[1] - self.grips()[index][1])

    def clone(self):
        import copy
        return copy.copy(self)

    def snapshot(self):
        """撤销用的深拷贝：默认浅拷贝会共享 pts 列表，必须深复制。"""
        import copy
        return copy.deepcopy(self)

    # -------------------------------------------------- 序列化
    def style_dict(self):
        return {"layer": self.layer, "color": self.color,
                "width": self.width, "linetype": self.linetype}

    def apply_style(self, d):
        self.layer = d.get("layer", self.layer) or "0"
        self.color = d.get("color", 0)
        self.width = d.get("width", 0.0)
        self.linetype = d.get("linetype", "")

    def to_dict(self):
        raise NotImplementedError

    @classmethod
    def from_dict(cls, d):
        raise NotImplementedError


class Line(Entity):
    kind = "line"

    def __init__(self, x1, y1, x2, y2, **kw):
        Entity.__init__(self, **kw)
        self.x1, self.y1, self.x2, self.y2 = float(x1), float(y1), float(x2), float(y2)

    @property
    def start(self):
        return (self.x1, self.y1)

    @property
    def end(self):
        return (self.x2, self.y2)

    def bbox(self):
        return (min(self.x1, self.x2), min(self.y1, self.y2),
                max(self.x1, self.x2), max(self.y1, self.y2))

    def points(self):
        return [(self.x1, self.y1), (self.x2, self.y2)]

    def hit(self, p, tol):
        return dist_pt_seg(p, self.start, self.end) <= tol

    def grips(self):
        return [self.start, ((self.x1 + self.x2) / 2.0, (self.y1 + self.y2) / 2.0), self.end]

    def move_by(self, dx, dy):
        self.x1 += dx; self.x2 += dx
        self.y1 += dy; self.y2 += dy

    def rotate_by(self, base, angle_deg):
        self.x1, self.y1 = geom.polar(base, geom.angle_of(base, self.start) + angle_deg,
                                       geom.dist_pt_pt(base, self.start))
        self.x2, self.y2 = geom.polar(base, geom.angle_of(base, self.end) + angle_deg,
                                      geom.dist_pt_pt(base, self.end))

    def scale_by(self, base, k):
        def sc(p):
            return (base[0] + (p[0] - base[0]) * k, base[1] + (p[1] - base[1]) * k)
        self.x1, self.y1 = sc(self.start)
        self.x2, self.y2 = sc(self.end)

    def set_grip(self, index, p):
        if index == 0:
            self.x1, self.y1 = p
        elif index == 2:
            self.x2, self.y2 = p
        else:
            self.move_by(p[0] - self.grips()[1][0], p[1] - self.grips()[1][1])

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "line", "x1": self.x1, "y1": self.y1,
                  "x2": self.x2, "y2": self.y2})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["x1"], d["y1"], d["x2"], d["y2"])
        e.apply_style(d)
        return e


class Circle(Entity):
    kind = "circle"

    def __init__(self, cx, cy, r, **kw):
        Entity.__init__(self, **kw)
        self.cx, self.cy, self.r = float(cx), float(cy), float(r)

    @property
    def center(self):
        return (self.cx, self.cy)

    def bbox(self):
        return circle_bbox(self.center, self.r)

    def points(self):
        return arc_points(self.center, self.r, 0.0, 360.0)

    def hit(self, p, tol):
        return abs(dist_pt_pt(p, self.center) - self.r) <= tol

    def grips(self):
        return [self.center, (self.cx + self.r, self.cy), (self.cx - self.r, self.cy),
                (self.cx, self.cy + self.r), (self.cx, self.cy - self.r)]

    def move_by(self, dx, dy):
        self.cx += dx; self.cy += dy

    def rotate_by(self, base, angle_deg):
        self.cx, self.cy = geom.polar(base, geom.angle_of(base, self.center) + angle_deg,
                                      geom.dist_pt_pt(base, self.center))

    def scale_by(self, base, k):
        self.cx = base[0] + (self.cx - base[0]) * k
        self.cy = base[1] + (self.cy - base[1]) * k
        self.r *= k

    def set_grip(self, index, p):
        if index == 0:
            self.cx, self.cy = p
        else:
            self.r = max(1e-6, geom.dist_pt_pt(self.center, p))

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "circle", "cx": self.cx, "cy": self.cy, "r": self.r})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["cx"], d["cy"], d["r"])
        e.apply_style(d)
        return e


class Arc(Entity):
    kind = "arc"

    def __init__(self, cx, cy, r, a1, a2, **kw):
        Entity.__init__(self, **kw)
        self.cx, self.cy, self.r = float(cx), float(cy), float(r)
        self.a1, self.a2 = float(a1), float(a2)     # 度，逆时针

    @property
    def center(self):
        return (self.cx, self.cy)

    @property
    def sweep(self):
        return arc_sweep(self.a1, self.a2)

    def bbox(self):
        return arc_bbox(self.center, self.r, self.a1, self.a2)

    def points(self):
        return arc_points(self.center, self.r, self.a1, self.a2)

    def hit(self, p, tol):
        return arc_hit(p, self.center, self.r, self.a1, self.a2, tol)

    def grips(self):
        a = math.radians(self.a1)
        b = math.radians(self.a2)
        return [self.center,
                (self.cx + self.r * math.cos(a), self.cy + self.r * math.sin(a)),
                (self.cx + self.r * math.cos(b), self.cy + self.r * math.sin(b))]

    def move_by(self, dx, dy):
        self.cx += dx; self.cy += dy

    def rotate_by(self, base, angle_deg):
        self.cx, self.cy = geom.polar(base, geom.angle_of(base, self.center) + angle_deg,
                                      geom.dist_pt_pt(base, self.center))
        self.a1 = geom.norm_deg(self.a1 + angle_deg)
        self.a2 = geom.norm_deg(self.a2 + angle_deg)

    def scale_by(self, base, k):
        self.cx = base[0] + (self.cx - base[0]) * k
        self.cy = base[1] + (self.cy - base[1]) * k
        self.r *= k

    def set_grip(self, index, p):
        if index == 0:
            self.cx, self.cy = p
        else:
            ang = geom.angle_of(self.center, p)
            self.r = max(1e-6, geom.dist_pt_pt(self.center, p))
            if index == 1:
                self.a1 = ang
            else:
                self.a2 = ang

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "arc", "cx": self.cx, "cy": self.cy, "r": self.r,
                  "a1": self.a1, "a2": self.a2})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["cx"], d["cy"], d["r"], d["a1"], d["a2"])
        e.apply_style(d)
        return e


class Rect(Entity):
    """矩形是"四个角点 + 闭合"的多段线形状，单独成类是为了命中和夹点更直觉。"""
    kind = "rect"

    def __init__(self, x1, y1, x2, y2, **kw):
        Entity.__init__(self, **kw)
        self.x1, self.y1, self.x2, self.y2 = float(x1), float(y1), float(x2), float(y2)

    @property
    def corners(self):
        x0, y0, x1, y1 = rect_norm(self.x1, self.y1, self.x2, self.y2)
        return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]

    def bbox(self):
        return rect_norm(self.x1, self.y1, self.x2, self.y2)

    def points(self):
        return self.corners + self.corners[:1]

    def is_closed(self):
        return True

    def hit(self, p, tol):
        x0, y0, x1, y1 = self.bbox()
        if p[0] < x0 - tol or p[0] > x1 + tol or p[1] < y0 - tol or p[1] > y1 + tol:
            return False
        return (dist_pt_polyline(p, self.corners, True) <= tol)

    def grips(self):
        c = self.corners
        return c + [((c[0][0] + c[2][0]) / 2.0, c[0][1]),
                    (c[1][0], (c[1][1] + c[3][1]) / 2.0),
                    ((c[0][0] + c[2][0]) / 2.0, c[2][1]),
                    (c[0][0], (c[0][1] + c[2][1]) / 2.0)]

    def move_by(self, dx, dy):
        self.x1 += dx; self.x2 += dx
        self.y1 += dy; self.y2 += dy

    def rotate_by(self, base, angle_deg):
        pts = [geom.polar(base, geom.angle_of(base, c) + angle_deg,
                          geom.dist_pt_pt(base, c)) for c in self.corners]
        bx = geom.bbox_points(pts)
        self.x1, self.y1, self.x2, self.y2 = bx

    def scale_by(self, base, k):
        pts = [(base[0] + (c[0] - base[0]) * k, base[1] + (c[1] - base[1]) * k)
               for c in self.corners]
        bx = geom.bbox_points(pts)
        self.x1, self.y1, self.x2, self.y2 = bx

    def set_grip(self, index, p):
        x0, y0, x1, y1 = self.bbox()
        if index == 0:
            x0, y0 = p
        elif index == 1:
            x1, y0 = p
        elif index == 2:
            x1, y1 = p
        elif index == 3:
            x0, y1 = p
        elif index == 4:
            y0 = p[1]
        elif index == 5:
            x1 = p[0]
        elif index == 6:
            y1 = p[1]
        else:
            x0 = p[0]
        self.x1, self.y1, self.x2, self.y2 = x0, y0, x1, y1

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "rect", "x1": self.x1, "y1": self.y1,
                  "x2": self.x2, "y2": self.y2})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["x1"], d["y1"], d["x2"], d["y2"])
        e.apply_style(d)
        return e


class Polyline(Entity):
    kind = "polyline"

    def __init__(self, pts, closed=False, **kw):
        Entity.__init__(self, **kw)
        self.pts = [(float(p[0]), float(p[1])) for p in pts]
        self.closed = bool(closed)

    def bbox(self):
        return bbox_points(self.pts)

    def points(self):
        return self.pts + self.pts[:1] if self.closed and len(self.pts) > 2 else list(self.pts)

    def is_closed(self):
        return self.closed

    def hit(self, p, tol):
        return dist_pt_polyline(p, self.pts, self.closed) <= tol

    def grips(self):
        return list(self.pts)

    def move_by(self, dx, dy):
        self.pts = [(p[0] + dx, p[1] + dy) for p in self.pts]

    def rotate_by(self, base, angle_deg):
        self.pts = [geom.polar(base, geom.angle_of(base, p) + angle_deg,
                               geom.dist_pt_pt(base, p)) for p in self.pts]

    def scale_by(self, base, k):
        self.pts = [(base[0] + (p[0] - base[0]) * k, base[1] + (p[1] - base[1]) * k)
                    for p in self.pts]

    def set_grip(self, index, p):
        if 0 <= index < len(self.pts):
            self.pts[index] = (float(p[0]), float(p[1]))

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "polyline", "pts": [list(p) for p in self.pts],
                  "closed": self.closed})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["pts"], closed=d.get("closed", False))
        e.apply_style(d)
        return e


class Text(Entity):
    kind = "text"

    def __init__(self, x, y, text, size=200.0, style="Standard", rot=0.0,
                 align=7, **kw):
        Entity.__init__(self, **kw)
        self.x, self.y = float(x), float(y)
        self.text = text
        self.size = float(size)      # 字高 mm
        self.style = style           # 文字样式名 / 字体族
        self.rot = float(rot)        # 旋转角，度
        # 对齐码（AutoCAD 1-9：上排 1/2/3、中排 4/5/6、下排 7/8/9），默认左下 7。
        self.align = int(align)

    def bbox(self):
        w = self.size * max(1, len(self.text)) * 0.62
        h = self.size * 1.1
        if abs(self.rot) < 1e-6:
            return (self.x, self.y, self.x + w, self.y + h)
        # 旋转时取四个角点的包围盒
        r = math.radians(self.rot)
        pts = []
        for dx, dy in ((0, 0), (w, 0), (w, h), (0, h)):
            pts.append((self.x + dx * math.cos(r) - dy * math.sin(r),
                        self.y + dx * math.sin(r) + dy * math.cos(r)))
        return bbox_points(pts)

    def points(self):
        x0, y0, x1, y1 = self.bbox()
        return [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]

    def is_closed(self):
        return True

    def grips(self):
        return [(self.x, self.y)]

    def move_by(self, dx, dy):
        self.x += dx; self.y += dy

    def rotate_by(self, base, angle_deg):
        self.x, self.y = geom.polar(base, geom.angle_of(base, (self.x, self.y)) + angle_deg,
                                    geom.dist_pt_pt(base, (self.x, self.y)))
        self.rot = geom.norm_deg(self.rot + angle_deg)

    def scale_by(self, base, k):
        self.x = base[0] + (self.x - base[0]) * k
        self.y = base[1] + (self.y - base[1]) * k
        self.size *= k

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "text", "x": self.x, "y": self.y, "text": self.text,
                  "size": self.size, "style": self.style, "rot": self.rot})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["x"], d["y"], d["text"], size=d.get("size", 200.0),
                style=d.get("style", "Standard"), rot=d.get("rot", 0.0))
        e.apply_style(d)
        return e


class Insert(Entity):
    """块参照：把块表里的实体按基点+缩放+旋转摆到图上。"""
    kind = "insert"

    def __init__(self, name, x, y, sx=1.0, sy=1.0, rot=0.0, **kw):
        Entity.__init__(self, **kw)
        self.name = name
        self.x, self.y = float(x), float(y)
        self.sx, self.sy, self.rot = float(sx), float(sy), float(rot)

    def bbox(self):
        # 没有块表就退化成基点一个小盒，避免上层炸掉
        return (self.x, self.y, self.x, self.y)

    def points(self):
        return [(self.x, self.y)]

    def hit(self, p, tol):
        return dist_pt_pt(p, (self.x, self.y)) <= tol

    def grips(self):
        return [(self.x, self.y)]

    def move_by(self, dx, dy):
        self.x += dx; self.y += dy

    def scale_by(self, base, k):
        self.x = base[0] + (self.x - base[0]) * k
        self.y = base[1] + (self.y - base[1]) * k
        self.sx *= k
        self.sy *= k

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "insert", "name": self.name, "x": self.x, "y": self.y,
                  "sx": self.sx, "sy": self.sy, "rot": self.rot})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["name"], d["x"], d["y"], sx=d.get("sx", 1.0),
                sy=d.get("sy", 1.0), rot=d.get("rot", 0.0))
        e.apply_style(d)
        return e


class Dimension(Entity):
    """线性标注：两条延伸线 + 一条尺寸线 + 长度文字。

    p1/p2 是被标注的两个原点；off 是尺寸线相对 p1p2 的有向垂直距离（mm）。
    """
    kind = "dimension"

    def __init__(self, p1, p2, off=300.0, **kw):
        Entity.__init__(self, **kw)
        self.p1 = (float(p1[0]), float(p1[1]))
        self.p2 = (float(p2[0]), float(p2[1]))
        self.off = float(off)

    @property
    def length(self):
        return geom.dist_pt_pt(self.p1, self.p2)

    def _basis(self):
        dx, dy = self.p2[0] - self.p1[0], self.p2[1] - self.p1[1]
        n = geom.length((dx, dy))
        if n < 1e-9:
            return (1.0, 0.0), (0.0, 1.0)
        u = (dx / n, dy / n)
        return u, (-u[1], u[0])

    def dim_points(self):
        """返回 (q1, q2, ext1_top, ext2_top, mid)：尺寸线两端、延伸线顶端、文字点。"""
        u, n = self._basis()
        q1 = (self.p1[0] + n[0] * self.off, self.p1[1] + n[1] * self.off)
        q2 = (self.p2[0] + n[0] * self.off, self.p2[1] + n[1] * self.off)
        ext = max(100.0, self.length * 0.06)
        t1 = (q1[0] + n[0] * ext, q1[1] + n[1] * ext)
        t2 = (q2[0] + n[0] * ext, q2[1] + n[1] * ext)
        mid = ((q1[0] + q2[0]) / 2.0, (q1[1] + q2[1]) / 2.0)
        return q1, q2, t1, t2, mid

    def bbox(self):
        q1, q2, t1, t2, mid = self.dim_points()
        return geom.bbox_points([self.p1, self.p2, q1, q2, t1, t2, mid])

    def points(self):
        q1, q2, t1, t2, _ = self.dim_points()
        return [self.p1, t1, q1, q2, t2, self.p2]

    def hit(self, p, tol):
        q1, q2, t1, t2, _ = self.dim_points()
        best = min(geom.dist_pt_seg(p, self.p1, t1),
                   geom.dist_pt_seg(p, self.p2, t2),
                   geom.dist_pt_seg(p, q1, q2))
        return best <= tol

    def grips(self):
        q1, q2, _, _, mid = self.dim_points()
        return [self.p1, self.p2, mid]

    def move_by(self, dx, dy):
        self.p1 = (self.p1[0] + dx, self.p1[1] + dy)
        self.p2 = (self.p2[0] + dx, self.p2[1] + dy)

    def rotate_by(self, base, angle_deg):
        self.p1 = geom.polar(base, geom.angle_of(base, self.p1) + angle_deg,
                             geom.dist_pt_pt(base, self.p1))
        self.p2 = geom.polar(base, geom.angle_of(base, self.p2) + angle_deg,
                             geom.dist_pt_pt(base, self.p2))

    def scale_by(self, base, k):
        self.p1 = (base[0] + (self.p1[0] - base[0]) * k,
                   base[1] + (self.p1[1] - base[1]) * k)
        self.p2 = (base[0] + (self.p2[0] - base[0]) * k,
                   base[1] + (self.p2[1] - base[1]) * k)
        self.off *= k

    def set_grip(self, index, p):
        if index == 0:
            self.p1 = (float(p[0]), float(p[1]))
        elif index == 1:
            self.p2 = (float(p[0]), float(p[1]))
        else:
            # 中点夹点：改垂直偏移
            u, n = self._basis()
            proj = (p[0] - self.p1[0]) * n[0] + (p[1] - self.p1[1]) * n[1]
            self.off = proj

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "dimension", "p1": list(self.p1), "p2": list(self.p2),
                  "off": self.off})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["p1"], d["p2"], off=d.get("off", 300.0))
        e.apply_style(d)
        return e


class Hatch(Entity):
    """区域剖面填充：边界多边形 + 剖面线角度 / 间距（Python 扫描线兜底，不依赖 SVG pattern）。"""
    kind = "hatch"

    def __init__(self, pts, angle=45.0, gap=120.0, **kw):
        Entity.__init__(self, **kw)
        self.pts = [(float(p[0]), float(p[1])) for p in pts]
        self.angle = float(angle)
        self.gap = float(gap)

    def bbox(self):
        return bbox_points(self.pts)

    def points(self):
        return self.pts + self.pts[:1]

    def is_closed(self):
        return True

    def hit(self, p, tol):
        if dist_pt_polyline(p, self.pts, True) <= tol:
            return True
        return point_in_polygon(p, self.pts)

    def grips(self):
        return [self.pts[0]] if self.pts else []

    def move_by(self, dx, dy):
        self.pts = [(x + dx, y + dy) for x, y in self.pts]

    def scale_by(self, base, k):
        self.pts = [(base[0] + (x - base[0]) * k, base[1] + (y - base[1]) * k)
                    for x, y in self.pts]
        self.gap *= k

    def rotate_by(self, base, angle_deg):
        self.pts = [geom.polar(base, geom.angle_of(base, p) + angle_deg,
                               geom.dist_pt_pt(base, p)) for p in self.pts]
        self.angle = geom.norm_deg(self.angle + angle_deg)

    def to_dict(self):
        d = self.style_dict()
        d.update({"kind": "hatch", "pts": [list(p) for p in self.pts],
                  "angle": self.angle, "gap": self.gap})
        return d

    @classmethod
    def from_dict(cls, d):
        e = cls(d["pts"], angle=d.get("angle", 45.0), gap=d.get("gap", 120.0))
        e.apply_style(d)
        return e


# 供文档/命令行按名字造实体
FACTORY = {
    "line": Line, "circle": Circle, "arc": Arc, "rect": Rect,
    "polyline": Polyline, "text": Text, "insert": Insert, "dimension": Dimension,
    "hatch": Hatch,
}
