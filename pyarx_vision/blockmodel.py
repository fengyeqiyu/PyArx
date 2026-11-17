# -*- coding: utf-8 -*-
"""PyArx 块模型：块定义 / 块参照 / 属性，按整体控件的方式承载几何变换。

CAD 的块矩阵是 6 元组 [a, b, c, d, e, f]：
    x' = a*x + c*y + e
    y' = b*x + d*y + f
所以 Xform 里 a=m[0], b=m[1], c=m[2], d=m[3], e=m[4], f=m[5]。

一个 BlockRef = 块名 + Xform + 属性列表：几何、文字、属性都挂在这一个整体上，
平移/旋转/缩放/镜像只改变 Xform，属性坐标跟随同一条矩阵，导出时一次性变换。
"""

import math

IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


class Xform(object):
    """2D 仿射变换（CAD 6 元组）。不可变、可组合、可求逆。"""

    __slots__ = ("a", "b", "c", "d", "e", "f")

    def __init__(self, a=1.0, b=0.0, c=0.0, d=1.0, e=0.0, f=0.0):
        self.a, self.b, self.c, self.d, self.e, self.f = (float(a), float(b), float(c),
                                                          float(d), float(e), float(f))

    # ---- 构造 ----
    @classmethod
    def from_cad(cls, m):
        if not m or len(m) != 6:
            return cls()
        return cls(*m)

    @classmethod
    def translate(cls, dx, dy):
        return cls(1.0, 0.0, 0.0, 1.0, dx, dy)

    @classmethod
    def scale(cls, sx, sy=None):
        return cls(sx, 0.0, 0.0, sx if sy is None else sy, 0.0, 0.0)

    @classmethod
    def rotate(cls, deg):
        r = math.radians(deg)
        cos, sin = math.cos(r), math.sin(r)
        return cls(cos, sin, -sin, cos, 0.0, 0.0)

    def to_cad(self):
        return [round(v, 6) for v in (self.a, self.b, self.c, self.d, self.e, self.f)]

    # ---- 组合：self ∘ other（先 other 再 self，对应块嵌套） ----
    def compose(self, other):
        a1, b1, c1, d1, e1, f1 = self.a, self.b, self.c, self.d, self.e, self.f
        a2, b2, c2, d2, e2, f2 = other.a, other.b, other.c, other.d, other.e, other.f
        return Xform(a1 * a2 + c1 * b2, b1 * a2 + d1 * b2,
                     a1 * c2 + c1 * d2, b1 * c2 + d1 * d2,
                     a1 * e2 + c1 * f2 + e1, b1 * e2 + d1 * f2 + f1)

    def inverse(self):
        det = self.a * self.d - self.b * self.c
        if abs(det) < 1e-12:
            return Xform()
        ia, ib = self.d / det, -self.b / det
        ic, idd = -self.c / det, self.a / det
        return Xform(ia, ib, ic, idd,
                     -(ia * self.e + ic * self.f), -(ib * self.e + idd * self.f))

    # ---- 作用于点 / 包围盒 ----
    def apply(self, x, y):
        return (self.a * x + self.c * y + self.e, self.b * x + self.d * y + self.f)

    def bounds(self, box):
        """把 local bbox 的四角变换后取 AABB —— 旋转/镜像/斜切都不会偏。"""
        x0, y0, x1, y1 = box
        pts = [self.apply(x0, y0), self.apply(x1, y0),
               self.apply(x1, y1), self.apply(x0, y1)]
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        return [min(xs), min(ys), max(xs), max(ys)]

    def scale_factor(self):
        """近似缩放（取两轴均值），用于字高换算。"""
        sx = math.hypot(self.a, self.b)
        sy = math.hypot(self.c, self.d)
        return (sx + sy) * 0.5

    def rotation_deg(self):
        return math.degrees(math.atan2(self.b, self.a))

    def is_identity(self):
        return (abs(self.a - 1) < 1e-9 and abs(self.d - 1) < 1e-9 and
                abs(self.b) < 1e-9 and abs(self.c) < 1e-9 and
                abs(self.e) < 1e-9 and abs(self.f) < 1e-9)

    def is_axis_aligned(self):
        """轴上对齐（只含平移/轴向缩放/90°倍数旋转）——可走快速路径。"""
        return (abs(self.b) < 1e-9 and abs(self.c) < 1e-9) or \
               (abs(self.a) < 1e-9 and abs(self.d) < 1e-9)

    def __repr__(self):
        return "Xform(%s)" % ", ".join("%.4g" % v for v in
                                       (self.a, self.b, self.c, self.d, self.e, self.f))


class Attrib(object):
    """块属性（ATTRIB）：文字内容 + 真实坐标 + 字高/旋转/对齐。"""

    __slots__ = ("tag", "text", "x", "y", "h", "rot", "ha", "va", "layer", "color", "inv")

    def __init__(self, tag="", text="", x=0.0, y=0.0, h=0.0, rot=0.0,
                 ha=0, va=0, layer=None, color=0, inv=False):
        self.tag, self.text = tag, text
        self.x, self.y, self.h, self.rot = float(x), float(y), float(h), float(rot)
        self.ha, self.va = int(ha or 0), int(va or 0)
        self.layer, self.color, self.inv = layer, int(color or 0), bool(inv)

    @classmethod
    def from_dict(cls, d):
        return cls(tag=d.get("tag") or "", text=d.get("txt") or "",
                   x=d.get("x") or 0.0, y=d.get("y") or 0.0, h=d.get("h") or 0.0,
                   rot=d.get("rot") or 0.0, ha=d.get("ha") or 0, va=d.get("va") or 0,
                   layer=d.get("ly"), color=d.get("ci") or 0, inv=d.get("inv") or False)

    def moved(self, x, y):
        return Attrib(self.tag, self.text, x, y, self.h, self.rot,
                      self.ha, self.va, self.layer, self.color, self.inv)

    def to_dict(self):
        return {"tag": self.tag, "txt": self.text, "x": self.x, "y": self.y,
                "h": self.h, "rot": self.rot, "ha": self.ha, "va": self.va,
                "ly": self.layer, "ci": self.color, "inv": self.inv}


class BlockDef(object):
    """块定义：块内坐标下的几何，以及嵌套子块。"""

    __slots__ = ("name", "lines", "plines", "circles", "arcs", "texts", "ins", "attdefs")

    def __init__(self, name, geom=None, attdefs=None):
        g = geom or {}
        self.name = name
        self.lines = g.get("lines") or []
        self.plines = g.get("plines") or []
        self.circles = g.get("circles") or []
        self.arcs = g.get("arcs") or []
        self.texts = g.get("texts") or []
        self.ins = g.get("ins") or []
        self.attdefs = attdefs or []

    @classmethod
    def from_cache(cls, name, d):
        return cls(name, d.get("geom") or {}, d.get("attdefs") or [])

    def local_bbox(self):
        """块内（未变换）包围盒；遍历嵌套子块时用它们各自的矩阵。"""
        xs, ys = [], []
        for ln in self.lines:
            xs += [ln[0], ln[2]]
            ys += [ln[1], ln[3]]
        for pl in self.plines:
            pts = pl.get("p") or []
            xs += pts[0::2]
            ys += pts[1::2]
        for cr in self.circles:
            cx, cy, r = cr[0], cr[1], cr[2]
            xs += [cx - r, cx + r]
            ys += [cy - r, cy + r]
        for ar in self.arcs:
            cx, cy, r = ar[0], ar[1], ar[2]
            xs += [cx - r, cx + r]
            ys += [cy - r, cy + r]
        for t in self.texts:
            xs += [t["x"], t["x"] + abs(t.get("h") or 0) * 0.6]
            ys += [t["y"], t["y"] + abs(t.get("h") or 0)]
        return [min(xs), min(ys), max(xs), max(ys)] if xs else [0, 0, 0, 0]


class BlockRef(object):
    """块参照 = 块名 + 变换矩阵 + 属性列表（一个整体控件）。"""

    __slots__ = ("name", "xf", "attribs", "layer", "color", "handle")

    def __init__(self, name, xf=None, attribs=None, layer=None, color=0, handle=None):
        self.name = name
        self.xf = xf if isinstance(xf, Xform) else Xform.from_cad(xf)
        self.attribs = [a if isinstance(a, Attrib) else Attrib.from_dict(a)
                        for a in (attribs or ())]
        self.layer, self.color, self.handle = layer, int(color or 0), handle

    @classmethod
    def from_cache(cls, b):
        return cls(b.get("n") or "", b.get("m"), b.get("attribs") or [],
                   b.get("l") or b.get("layer"), b.get("ci") or 0, b.get("handle"))

    # ---- 变换 ----
    def transformed(self, xf):
        """在原有变换之上再套一层（整体移动/旋转/缩放），属性坐标一起跟随。"""
        return BlockRef(self.name, xf.compose(self.xf),
                        [a.moved(*self.xf.apply(a.x, a.y)) if False else a
                         for a in self.attribs], self.layer, self.color, self.handle)

    def world_bbox(self, defs):
        """块定义局部 bbox 经本参照矩阵变换后的世界 AABB。"""
        bd = defs.get(self.name)
        if bd is None:
            return [0, 0, 0, 0]
        return self.xf.bounds(bd.local_bbox())

    def world_attribs(self):
        """属性的世界坐标。DWG 里 ATTRIB 的坐标已经是 WCS，这里仅作一致性校验入口。"""
        return [(a.text, a.x, a.y, a.h, a.rot, a.ha, a.va) for a in self.attribs]


def build_defs(block_defs):
    return {n: BlockDef.from_cache(n, d) for n, d in (block_defs or {}).items()}


def ref_bbox(ref, defs, limit=20000000.0):
    """块参照的稳妥包围盒：先按矩阵变换；数值离谱（块内坐标带世界级残差、
    或引用块自身被缩放数万倍）时退回"以插入点为中心、按定义尺寸×缩放"的保守框，
    避免单个坏框把整幅取景撑爆。"""
    bd = defs.get(ref.name)
    if bd is None:
        return [0, 0, 0, 0]
    box = ref.xf.bounds(bd.local_bbox())
    w, h = box[2] - box[0], box[3] - box[1]
    px, py = ref.xf.e
    if (w > limit or h > limit or
            abs((box[0] + box[2]) * 0.5 - px) > limit or
            abs((box[1] + box[3]) * 0.5 - py) > limit):
        k = min(abs(ref.xf.scale_factor()), 1e6) or 1.0
        lb = bd.local_bbox()
        lw, lh = (lb[2] - lb[0]) * k, (lb[3] - lb[1]) * k
        if lw > limit or lh > limit:
            lw = lh = 0.0
        return [px - lw * 0.5, py - lh * 0.5, px + lw * 0.5, py + lh * 0.5]
    return box
