# -*- coding: utf-8 -*-
"""render —— 图元渲染核心（模块 3）

只做一件事：把「实体 + 符号表」翻成画布能吃的 SVG。
  · 坐标：CAD 系（y 向上）→ 屏幕系（y 向下），并按 SCREEN_UNIT 换算成画布单位
  · 颜色/线宽/线型：实体显式值优先，否则随层
  · 线宽与字高按屏幕单位给，缩放时比例一致，不会缩成一根头发丝
换渲染目标（PNG 出图 / DXF 导出 / 打印）时，只换这一层，实体和界面都不动。
"""
import math
import re

from geom import arc_points

# 文档单位（mm）→ 画布屏幕单位：1 mm = 0.1 屏幕单位。
# 一份 12000×8000 mm 的图在画布里是 1200×800，滚轮缩放的舒服区间正好覆盖它。
SCREEN_UNIT = 0.1
REF_PX = 1200.0      # 任何图纸的幅面长边都折算到这么多画布单位（见 Document.screen_unit）

# AutoCAD 深色主题下的画布底色与索引色。
# 注意：SVG 的颜色只认 #RRGGBB / #RRGGBBAA，不认 MFC 的 #AARRGGBB 写法，
# 写成 #FF2B2F33 会被判为非法色，整块底色渲染成红色。
CANVAS_BG = "#2E3237"

ACI = {
    1: "#FF6B6B", 2: "#E8C05A", 3: "#5FD08A", 4: "#4FD0D0", 5: "#6BA5FF",
    6: "#D08AE0", 7: "#F2F4F6", 8: "#9AA0A8", 9: "#C0C6CE",
    30: "#E08A4A", 250: "#3A3A3A", 251: "#4A4A4A", 252: "#606060",
    253: "#8A8A8A", 254: "#C8C8C8",
}
DEFAULT_COLOR = "#E6E8EB"


def aci_color(idx, fallback=DEFAULT_COLOR):
    try:
        return ACI.get(int(idx), fallback)
    except Exception:
        return fallback


class ViewTransform(object):
    """世界(mm) → 画布(屏幕单位) 的转换，同时负责给出实体的有效样式。"""

    def __init__(self, symbols, view, zoom=1.0, unit=None, absolute=False):
        self.symbols = symbols
        self.x0, self.y0, self.x1, self.y1 = view
        # u = 本图纸的 px/世界单位。不同图纸的世界范围差几个数量级（英寸小图
        # 91 单位、城市坐标上百万），用统一常数会让 SVG 要么被放大成巨块、要么
        # 画成几十万像素宽把画布拖垮，所以由 Document.screen_unit 按幅面归一化。
        self.u = float(unit or SCREEN_UNIT)
        self.zoom = float(zoom or 1.0)   # 画布缩放系数（屏幕 = 世界差 × u × zoom）
        # absolute=True：场景坐标只由世界坐标决定（不平移、不随视图原点变），
        # 于是整图只序列化/解析一次，之后平移缩放全交给画布 setDwgView。
        self.absolute = bool(absolute)
        # 参照宽度（SVG 用户单位）：线宽/字号的下限按它等比缩放。
        self.ref_w = max(1e-6, (self.x1 - self.x0) * self.u)
        self.k = self.ref_w / REF_PX     # 归一化后恒 ≈1；旧图纸 k==1 观感不变

    # --- 坐标 ---
    def px(self, x):
        return x * self.u if self.absolute else (x - self.x0) * self.u

    def py(self, y):
        return -y * self.u if self.absolute else (self.y1 - y) * self.u   # CAD y 向上 → 屏幕 y 向下

    def plen(self, v):
        return v * self.u

    def local(self, base):
        """块局部坐标的变换副本：原点落在块基点，y 仍向屏幕下。

        块内容只序列化一次（见 _block_svg），插入时用 <g transform> 摆放，
        不再按每个块参照 clone 一遍块内实体。
        """
        t = ViewTransform.__new__(ViewTransform)
        t.symbols = self.symbols
        t.x0 = float(base[0])
        t.y1 = float(base[1])
        t.x1 = t.x0 + 1.0
        t.y0 = t.y1 - 1.0
        t.u = self.u
        t.zoom = 1.0
        t.ref_w = self.ref_w
        t.k = self.k
        t.absolute = self.absolute
        return t

    # --- 样式 ---
    def layer_of(self, e):
        return self.symbols.layers.get(e.layer) or self.symbols.layers.ensure(e.layer)

    def color_of(self, e):
        lay = self.layer_of(e)
        return aci_color(e.color if e.color else lay.color)

    def width_of(self, e):
        lay = self.layer_of(e)
        wmm = e.width or lay.width
        base = 1.0 * self.k               # 细线基准宽（随视图尺度缩放）
        if not wmm:                       # 未给线宽 → 细线
            return base
        return max(base, min(4.0 * self.k, wmm * self.u * 4.0))

    def linetype_of(self, e):
        lay = self.layer_of(e)
        name = e.linetype or lay.linetype
        lt = self.symbols.linetypes.get(name)
        return lt

    def dash_of(self, e):
        lt = self.linetype_of(e)
        if lt is None or lt.continuous:
            return ""
        el = lt.elements
        if len(el) % 2 != 0:
            el = el + [0.0]
        scale = self.u * 10.0            # 线型元素按图纸长度给，放大 10 倍才好辨认
        out = []
        for i, v in enumerate(el):
            seg = max(0.6, v * scale) if i % 2 == 0 else max(0.6, v * scale)
            out.append("%.2f" % seg)
        return ' stroke-dasharray="%s"' % ",".join(out)

    def stroke_style(self, e):
        return ' stroke="%s" stroke-width="%.2f" fill="none"%s' % (
            self.color_of(e), self.width_of(e), self.dash_of(e))

    def fill_style(self, e):
        return ' fill="%s" stroke="none"' % self.color_of(e)


def _svg_text_extra(vt, e):
    """文字要拿到样式表里的字高/字体，所以单独走一条。"""
    st = vt.symbols.textstyles.get(e.style)
    if st is None:
        # e.style 直接是字体族（DWG 适配器透传）时优先用；否则回退 Standard。
        if e.style and e.style != "Standard":
            fam = e.style
            size_mm = e.size
        else:
            st = vt.symbols.textstyles.get("Standard")
            size_mm = e.size or (st.height if st else 0.0) or (vt.y1 - vt.y0) * 0.025
            fam = st.font if st else "Microsoft YaHei"
    else:
        size_mm = e.size or (st.height if st else 0.0) or (vt.y1 - vt.y0) * 0.025
        fam = st.font
    size = vt.plen(size_mm)
    return ' font-size="%.2f" font-family="%s"' % (max(1.0 * vt.k, size), fam)


def _text_anchor(align):
    """对齐码 1-9 → SVG 水平锚点（左/中/右）。"""
    if align in (3, 6, 9):
        return "end"
    if align in (2, 5, 8):
        return "middle"
    return "start"


def _esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def entity_svg(e, vt, cache=None):
    """单个实体 → SVG 片段。这里是唯一按实体类型分派的地方。"""
    kind = e.kind
    if kind == "line":
        return '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f"%s/>' % (
            vt.px(e.x1), vt.py(e.y1), vt.px(e.x2), vt.py(e.y2), vt.stroke_style(e))
    if kind == "circle":
        return '<circle cx="%.3f" cy="%.3f" r="%.3f"%s/>' % (
            vt.px(e.cx), vt.py(e.cy), vt.plen(e.r), vt.stroke_style(e))
    if kind == "arc":
        p1 = arc_points(e.center, e.r, e.a1, e.a1)[0]
        p2 = arc_points(e.center, e.r, e.a2, e.a2)[0]
        large = 1 if e.sweep > 180 else 0
        return '<path d="M %.3f %.3f A %.3f %.3f 0 %d 0 %.3f %.3f"%s/>' % (
            vt.px(p1[0]), vt.py(p1[1]), vt.plen(e.r), vt.plen(e.r), large,
            vt.px(p2[0]), vt.py(p2[1]), vt.stroke_style(e))
    if kind in ("rect", "polyline"):
        pts = e.points()
        d = ["M %.3f %.3f" % (vt.px(pts[0][0]), vt.py(pts[0][1]))]
        for p in pts[1:]:
            d.append("L %.3f %.3f" % (vt.px(p[0]), vt.py(p[1])))
        if e.is_closed() and len(pts) > 2:
            d.append("Z")
        return '<path d="%s"%s/>' % (" ".join(d), vt.stroke_style(e))
    if kind == "text":
        anchor = _text_anchor(getattr(e, "align", 7))
        return '<text x="%.3f" y="%.3f" text-anchor="%s"%s%s>%s</text>' % (
            vt.px(e.x), vt.py(e.y), anchor, vt.fill_style(e),
            _svg_text_extra(vt, e), _esc(e.text))
    if kind == "dimension":
        q1, q2, t1, t2, mid = e.dim_points()
        col = vt.color_of(e)
        w = vt.width_of(e)
        parts = [
            '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" '
            'stroke-width="%.2f" fill="none"/>' % (
                vt.px(e.p1[0]), vt.py(e.p1[1]), vt.px(t1[0]), vt.py(t1[1]), col, w),
            '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" '
            'stroke-width="%.2f" fill="none"/>' % (
                vt.px(e.p2[0]), vt.py(e.p2[1]), vt.px(t2[0]), vt.py(t2[1]), col, w),
            '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" '
            'stroke-width="%.2f" fill="none"/>' % (
                vt.px(q1[0]), vt.py(q1[1]), vt.px(q2[0]), vt.py(q2[1]), col, w),
        ]
        # 两端箭头：沿尺寸线方向的小三角（用短线模拟，避免填充路径兼容问题）
        ux = (q2[0] - q1[0])
        uy = (q2[1] - q1[1])
        ln = math.hypot(ux, uy) or 1.0
        ux, uy = ux / ln, uy / ln
        ah = max(4.0 * vt.k, vt.plen(max(60.0, e.length * 0.04)))
        for q in (q1, q2):
            sx = -ux if q is q1 else ux
            sy = -uy if q is q1 else uy
            nx, ny = -uy, ux
            a = (q[0] + sx * ah, q[1] + sy * ah)
            b = (q[0] + nx * ah * 0.35, q[1] + ny * ah * 0.35)
            c = (q[0] - nx * ah * 0.35, q[1] - ny * ah * 0.35)
            parts.append('<path d="M %.3f %.3f L %.3f %.3f L %.3f %.3f Z" fill="%s"/>'
                         % (vt.px(a[0]), vt.py(a[1]), vt.px(b[0]), vt.py(b[1]),
                            vt.px(c[0]), vt.py(c[1]), col))
        label = "%.1f" % e.length
        size = max(6.0 * vt.k, vt.plen(160.0))
        parts.append('<text x="%.3f" y="%.3f" font-size="%.2f" fill="%s" '
                     'text-anchor="middle">%s</text>'
                     % (vt.px(mid[0]), vt.py(mid[1]) - 2.0, size, col, _esc(label)))
        return "".join(parts)
    if kind == "hatch":
        col = vt.color_of(e)
        w = vt.width_of(e)
        th = math.radians(e.angle)
        ux, uy = math.cos(th), math.sin(th)
        nx, ny = -uy, ux
        pts = e.pts
        cs = [nx * p[0] + ny * p[1] for p in pts]
        cmin, cmax = min(cs), max(cs)
        parts = []
        c = cmin
        while c <= cmax + 1e-6:
            hits = []
            m = len(pts)
            for i in range(m):
                a = pts[i]
                b = pts[(i + 1) % m]
                den = nx * (b[0] - a[0]) + ny * (b[1] - a[1])
                if abs(den) < 1e-9:
                    continue
                s = (c - (nx * a[0] + ny * a[1])) / den
                if -1e-6 <= s <= 1.0 + 1e-6:
                    hx = a[0] + s * (b[0] - a[0])
                    hy = a[1] + s * (b[1] - a[1])
                    hits.append((ux * hx + uy * hy, (hx, hy)))
            hits.sort(key=lambda t: t[0])
            for k in range(0, len(hits) - 1, 2):
                p1, p2 = hits[k][1], hits[k + 1][1]
                parts.append(
                    '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" '
                    'stroke="%s" stroke-width="%.2f" fill="none"/>' % (
                        vt.px(p1[0]), vt.py(p1[1]), vt.px(p2[0]), vt.py(p2[1]),
                        col, w))
            c += e.gap
        return "".join(parts)
    if kind == "insert":
        # 块参照：块内容只序列化一次（_block_svg 缓存），这里只发一个 <g transform>。
        # 原来每个块参照都把块内实体 clone 后逐个序列化，2742 图元的图纸会摊成
        # 上万元素、十几 MB SVG，画布每帧解析就卡死。
        blk = vt.symbols.blocks.get(e.name)
        if blk is None:
            return '<circle cx="%.3f" cy="%.3f" r="%.3f"%s/>' % (
                vt.px(e.x), vt.py(e.y), vt.plen(e.size if hasattr(e, "size") else 20.0),
                vt.stroke_style(e))
        frag = _block_svg(blk, vt, cache)
        if not frag:
            return ""
        sx = float(getattr(e, "sx", 1.0) or 1.0)
        sy = float(getattr(e, "sy", 1.0) or sx)
        rot = float(getattr(e, "rot", 0.0) or 0.0)
        xf = "translate(%.2f %.2f)" % (vt.px(e.x), vt.py(e.y))
        if rot:
            # 世界是 y 向上、逆时针；画布 y 向下，旋转角要反号
            xf += " rotate(%.3f)" % (-rot)
        if sx != 1.0 or sy != 1.0:
            xf += " scale(%.4f %.4f)" % (sx, sy)
        return '<g transform="%s">%s</g>' % (xf, frag)
    return ""


def _block_svg(blk, vt, cache):
    """块定义 → 块局部坐标的 SVG 片段（同名块只算一次，供所有块参照复用）。"""
    if cache is None:
        frag = _build_block_svg(blk, vt, None)
        return frag
    frag = cache.get(blk.name)
    if frag is None:
        frag = _build_block_svg(blk, vt, cache)
        cache[blk.name] = frag
    return frag


def _build_block_svg(blk, vt, cache):
    lvt = vt.local(blk.base)
    parts = []
    for sub in blk.entities:
        frag = entity_svg(sub, lvt, cache)
        if frag:
            parts.append(frag)
    return "".join(parts)


def transform_insert(sub, blk, ins):
    """块内实体 → 世界坐标的一次拷贝（不改原文，块可以反复插入）。"""
    copy = sub.clone()
    bx, by = blk.base
    r = math.radians(ins.rot)

    def xf(p):
        dx, dy = (p[0] - bx) * ins.sx, (p[1] - by) * ins.sy
        return (ins.x + dx * math.cos(r) - dy * math.sin(r),
                ins.y + dx * math.sin(r) + dy * math.cos(r))

    if copy.kind == "line":
        copy.x1, copy.y1 = xf((copy.x1, copy.y1))
        copy.x2, copy.y2 = xf((copy.x2, copy.y2))
    elif copy.kind in ("circle", "arc"):
        copy.cx, copy.cy = xf((copy.cx, copy.cy))
        copy.r *= max(abs(ins.sx), abs(ins.sy))
        if copy.kind == "arc":
            copy.a1 += ins.rot
            copy.a2 += ins.rot
    elif copy.kind in ("rect", "polyline"):
        copy.pts = [xf(p) for p in copy.points()[:-1] if True] if copy.kind == "polyline" else None
        if copy.kind == "rect":
            x0, y0 = xf((copy.x1, copy.y1))
            x1, y1 = xf((copy.x2, copy.y2))
            copy.x1, copy.y1, copy.x2, copy.y2 = min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)
    elif copy.kind == "text":
        copy.x, copy.y = xf((copy.x, copy.y))
        copy.rot += ins.rot
        copy.size *= max(abs(ins.sx), abs(ins.sy))
    return copy


def grid_svg(vt, view, step_mm, color="#3A4046"):
    """栅格：按世界坐标铺线，随缩放/平移一起被 dwgView 变换。"""
    x0, y0, x1, y1 = view
    if step_mm <= 0:
        return ""
    parts = []
    x = math.ceil(x0 / step_mm) * step_mm
    while x <= x1:
        parts.append('<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" '
                     'stroke-width="0.6" fill="none"/>'
                     % (vt.px(x), vt.py(y0), vt.px(x), vt.py(y1), color))
        x += step_mm
    y = math.ceil(y0 / step_mm) * step_mm
    while y <= y1:
        parts.append('<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="%s" '
                     'stroke-width="0.6" fill="none"/>'
                     % (vt.px(x0), vt.py(y), vt.px(x1), vt.py(y), color))
        y += step_mm
    return "".join(parts)


def grips_svg(vt, entities, color="#E8B23D"):
    """选中实体的夹点：小方块（世界坐标，随视图缩放）。"""
    half = max(2.0, vt.plen(22.0))
    parts = []
    for e in entities:
        for gx, gy in e.grips():
            sx, sy = vt.px(gx), vt.py(gy)
            parts.append('<rect x="%.3f" y="%.3f" width="%.2f" height="%.2f" '
                         'fill="%s" stroke="#1B1D20" stroke-width="0.8"/>'
                         % (sx - half, sy - half, half * 2, half * 2, color))
    return "".join(parts)


def rubber_svg(vt, p1, p2, color="#E8B23D"):
    """窗口选择橡皮框（世界坐标）。"""
    x0, y0, x1, y1 = geom_rect_norm(p1[0], p1[1], p2[0], p2[1])
    return ('<rect x="%.3f" y="%.3f" width="%.3f" height="%.3f" fill="%s22" '
             'stroke="%s" stroke-width="1.0" stroke-dasharray="4 3"/>'
             % (vt.px(x0), vt.py(y1), vt.plen(x1 - x0), vt.plen(y1 - y0),
                color, color))


def geom_rect_norm(x1, y1, x2, y2):
    return (min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))


def crosshair_svg(vt, p, color="#C9D1D9"):
    """CAD 十字光标：贯穿视口的横/竖线 + 中心小空心方框。

    框尺寸与线宽按当前缩放反算成屏幕固定像素，不随视图放大缩小（CAD 标准）。
    """
    cx, cy = p
    x0, y0, x1, y1 = vt.x0, vt.y0, vt.x1, vt.y1
    zoom = vt.zoom if vt.zoom > 1e-6 else 1.0
    h = 6.0 / (zoom * vt.u)               # 框半边长：固定 6px，反算回世界单位
    sw = 1.0 / zoom                      # 线宽：固定 1px，反算回 SVG 单位
    stroke = 'stroke="%s" stroke-width="%.2f" fill="none"' % (color, sw)
    parts = [
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(x0), vt.py(cy), vt.px(x1), vt.py(cy), stroke),
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(cx), vt.py(y0), vt.px(cx), vt.py(y1), stroke),
        # 中心空心方框：四条短线，交点在框正中心
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(cx - h), vt.py(cy + h), vt.px(cx + h), vt.py(cy + h), stroke),
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(cx - h), vt.py(cy - h), vt.px(cx + h), vt.py(cy - h), stroke),
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(cx - h), vt.py(cy - h), vt.px(cx - h), vt.py(cy + h), stroke),
        '<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" %s/>' % (
            vt.px(cx + h), vt.py(cy - h), vt.px(cx + h), vt.py(cy + h), stroke),
    ]
    return "".join(parts)


def to_svg(symbols, entities, view, grid=False, grid_step=0.0,
           selection=None, rubber=None, overlays=None, cursor=None, zoom=1.0,
           unit=None, cull=True, absolute=False):
    """符号表 + 实体表 → 一份完整 SVG 文本。

    首帧先铺一块比视口大得多的底色矩形：XDwgCanvas 的 Background 属性在宿主里
    不生效（画布永远浅底），深色主题只能靠 SVG 自己带背景。矩形给得足够大，
    平移/缩放出图幅也不会露白边。

    cull=True 时跳过落在视口外的实体：整图 2700 图元摊成上万元素、十几 MB 的
    SVG，画布每帧解析就是卡顿的主因，放大后只画看得见的那些。
    """
    vt = ViewTransform(symbols, view, zoom=zoom, unit=unit, absolute=absolute)
    vw = vt.plen(view[2] - view[0])
    vh = vt.plen(view[3] - view[1])
    parts = ['<?xml version="1.0" encoding="utf-8"?>',
             '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %.1f %.1f" '
             'width="%.1f" height="%.1f">' % (vw, vh, vw, vh),
             '<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"/>'
             % (vt.px(view[0]) - vw * 4, vt.py(view[3]) - vh * 4, vw * 9, vh * 9, CANVAS_BG)]
    if grid:
        parts.append(grid_svg(vt, view, grid_step))
    cache = {}
    margin = 24.0 / max(vt.u, 1e-12)      # 视口外留 24px 余量
    lo_x, hi_x = view[0] - margin, view[2] + margin
    lo_y, hi_y = view[1] - margin, view[3] + margin
    for e in entities:
        if cull:
            b = e.bbox()
            if b[2] < lo_x or b[0] > hi_x or b[3] < lo_y or b[1] > hi_y:
                continue
        frag = entity_svg(e, vt, cache)
        if frag:
            parts.append(frag)
    if selection:
        parts.append(grips_svg(vt, selection))
    if rubber is not None:
        parts.append(rubber_svg(vt, rubber[0], rubber[1]))
    if overlays:
        parts.extend(overlays)
    if cursor is not None:
        parts.append(crosshair_svg(vt, cursor))
    parts.append("</svg>")
    return "\n".join(parts)


# ============================ 第二种前端语言：<XScene> 显示列表 ============================
# 和上面 *_svg 那套一一对应：同样的实体分派、同样的视口裁剪、同样的块复用，只是把
# "拼 SVG 文本" 换成 "拼 XML 场景节点"。画布那头直接扫标签建场景（parseXmlSceneText），
# 不再做 SVG 词法 + nanosvg 二次解析，万级图元省掉一整趟序列化/解析。
#
#   <XScene Width="950" Height="613">
#     <XLine X1="10" Y1="20" X2="300" Y2="20" Stroke="#F2F4F6" Width="1.2"/>
#     <XCircle Cx="100" Cy="100" R="40" Stroke="#FF6B6B"/>
#     <XArc Cx="100" Cy="100" R="40" Start="0" End="90" Stroke="#FF6B6B"/>
#     <XPolyline Points="10,10 200,10 200,80 10,80" Closed="1" Stroke="#F2F4F6"/>
#     <XText X="10" Y="30" Size="14" Color="#F2F4F6" Anchor="middle">门厅说明</XText>
#     <XGroup Transform="translate(300 200) rotate(-30) scale(0.5 0.5)">…</XGroup>
#   </XScene>


def _xe(s):
    """XML 文本 / 属性值转义。"""
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def _xf(v):
    """场景数值：统一两位小数，顺手把 -0.00 归一成 0.00（免得下游拿到负零）。"""
    if abs(v) < 0.005:
        v = 0.0
    return "%.2f" % v


def _dash_pair(vt, e):
    """线型 → (实线段, 空档)；连续线返回 None。"""
    s = vt.dash_of(e)
    if not s:
        return None
    nums = [float(t) for t in re.findall(r"[0-9]+(?:\.[0-9]+)?", s)]
    if not nums:
        return None
    on = nums[0]
    off = nums[1] if len(nums) > 1 else nums[0]
    return max(0.5, on), max(0.5, off)


def _xstroke(vt, e, color=None, width=None):
    """描边属性：Stroke/Width/Dash（= SVG 的 stroke / stroke-width / stroke-dasharray）。"""
    col = color if color is not None else vt.color_of(e)
    w = width if width is not None else vt.width_of(e)
    out = ' Stroke="%s" Width="%s"' % (col, _xf(w))
    pair = _dash_pair(vt, e)
    if pair:
        out += ' Dash="%s,%s"' % (_xf(pair[0]), _xf(pair[1]))
    return out


def _xtext_size_family(vt, e):
    """文字字号与字体族（与 _svg_text_extra 同口径）。"""
    st = vt.symbols.textstyles.get(e.style)
    if st is None:
        if e.style and e.style != "Standard":
            fam = e.style
            size_mm = e.size
        else:
            st = vt.symbols.textstyles.get("Standard")
            size_mm = e.size or (st.height if st else 0.0) or (vt.y1 - vt.y0) * 0.025
            fam = st.font if st else "Microsoft YaHei"
    else:
        size_mm = e.size or (st.height if st else 0.0) or (vt.y1 - vt.y0) * 0.025
        fam = st.font
    return max(1.0 * vt.k, vt.plen(size_mm)), fam


def _xtext(x, y, text, size, color, anchor="start", rotate=0.0, family="Microsoft YaHei"):
    out = ('<XText X="%s" Y="%s" Size="%s" Color="%s" Anchor="%s" Family="%s"'
           % (_xf(x), _xf(y), _xf(size), color, anchor, _xe(family)))
    if abs(rotate) > 0.01:
        out += ' Rotate="%s"' % _xf(rotate)
    return out + ">%s</XText>" % _xe(text)


def _xline(x1, y1, x2, y2, col, w):
    return ('<XLine X1="%s" Y1="%s" X2="%s" Y2="%s" Stroke="%s" Width="%s"/>'
            % (_xf(x1), _xf(y1), _xf(x2), _xf(y2), col, _xf(w)))


def entity_xml(e, vt, cache=None):
    """单个实体 → XML 场景节点。与 entity_svg 一对一（同一套坐标/颜色/线型）。"""
    kind = e.kind
    if kind == "line":
        return ('<XLine X1="%s" Y1="%s" X2="%s" Y2="%s"%s/>'
                % (_xf(vt.px(e.x1)), _xf(vt.py(e.y1)),
                   _xf(vt.px(e.x2)), _xf(vt.py(e.y2)), _xstroke(vt, e)))
    if kind == "circle":
        return ('<XCircle Cx="%s" Cy="%s" R="%s"%s/>'
                % (_xf(vt.px(e.cx)), _xf(vt.py(e.cy)), _xf(vt.plen(e.r)), _xstroke(vt, e)))
    if kind == "arc":
        cx, cy = e.center
        return ('<XArc Cx="%s" Cy="%s" R="%s" Start="%s" End="%s"%s/>'
                % (_xf(vt.px(cx)), _xf(vt.py(cy)), _xf(vt.plen(e.r)),
                   _xf(e.a1), _xf(e.a2), _xstroke(vt, e)))
    if kind in ("rect", "polyline"):
        pts = e.points()
        flat = " ".join("%s,%s" % (_xf(vt.px(p[0])), _xf(vt.py(p[1]))) for p in pts)
        closed = "1" if (e.is_closed() and len(pts) > 2) else "0"
        return '<XPolyline Points="%s" Closed="%s"%s/>' % (flat, closed, _xstroke(vt, e))
    if kind == "text":
        size, fam = _xtext_size_family(vt, e)
        return _xtext(vt.px(e.x), vt.py(e.y), e.text, size, vt.color_of(e),
                      anchor=_text_anchor(getattr(e, "align", 7)),
                      rotate=-(getattr(e, "rot", 0.0) or 0.0), family=fam)
    if kind == "dimension":
        q1, q2, t1, t2, mid = e.dim_points()
        col = vt.color_of(e)
        w = vt.width_of(e)
        parts = [
            _xline(vt.px(e.p1[0]), vt.py(e.p1[1]), vt.px(t1[0]), vt.py(t1[1]), col, w),
            _xline(vt.px(e.p2[0]), vt.py(e.p2[1]), vt.px(t2[0]), vt.py(t2[1]), col, w),
            _xline(vt.px(q1[0]), vt.py(q1[1]), vt.px(q2[0]), vt.py(q2[1]), col, w),
        ]
        ux = q2[0] - q1[0]
        uy = q2[1] - q1[1]
        ln = math.hypot(ux, uy) or 1.0
        ux, uy = ux / ln, uy / ln
        ah = max(4.0 * vt.k, vt.plen(max(60.0, e.length * 0.04)))
        for q in (q1, q2):
            sx = -ux if q is q1 else ux
            sy = -uy if q is q1 else uy
            nx, ny = -uy, ux
            a = (q[0] + sx * ah, q[1] + sy * ah)
            b = (q[0] + nx * ah * 0.35, q[1] + ny * ah * 0.35)
            c = (q[0] - nx * ah * 0.35, q[1] - ny * ah * 0.35)
            parts.append('<XPolygon Points="%s,%s %s,%s %s,%s" Fill="%s"/>'
                         % (_xf(vt.px(a[0])), _xf(vt.py(a[1])),
                            _xf(vt.px(b[0])), _xf(vt.py(b[1])),
                            _xf(vt.px(c[0])), _xf(vt.py(c[1])), col))
        label = "%.1f" % e.length
        size = max(6.0 * vt.k, vt.plen(160.0))
        parts.append(_xtext(vt.px(mid[0]), vt.py(mid[1]) - 2.0, label, size, col,
                            anchor="middle"))
        return "".join(parts)
    if kind == "hatch":
        col = vt.color_of(e)
        w = vt.width_of(e)
        th = math.radians(e.angle)
        ux, uy = math.cos(th), math.sin(th)
        nx, ny = -uy, ux
        pts = e.pts
        cs = [nx * p[0] + ny * p[1] for p in pts]
        cmin, cmax = min(cs), max(cs)
        parts = []
        c = cmin
        while c <= cmax + 1e-6:
            hits = []
            m = len(pts)
            for i in range(m):
                a = pts[i]
                b = pts[(i + 1) % m]
                den = nx * (b[0] - a[0]) + ny * (b[1] - a[1])
                if abs(den) < 1e-9:
                    continue
                s = (c - (nx * a[0] + ny * a[1])) / den
                if -1e-6 <= s <= 1.0 + 1e-6:
                    hx = a[0] + s * (b[0] - a[0])
                    hy = a[1] + s * (b[1] - a[1])
                    hits.append((ux * hx + uy * hy, (hx, hy)))
            hits.sort(key=lambda t: t[0])
            for k in range(0, len(hits) - 1, 2):
                p1, p2 = hits[k][1], hits[k + 1][1]
                parts.append(_xline(vt.px(p1[0]), vt.py(p1[1]),
                                    vt.px(p2[0]), vt.py(p2[1]), col, w))
            c += e.gap
        return "".join(parts)
    if kind == "insert":
        blk = vt.symbols.blocks.get(e.name)
        if blk is None:
            return ('<XCircle Cx="%s" Cy="%s" R="%s"%s/>'
                    % (_xf(vt.px(e.x)), _xf(vt.py(e.y)),
                       _xf(vt.plen(e.size if hasattr(e, "size") else 20.0)), _xstroke(vt, e)))
        frag = _block_xml(blk, vt, cache)
        if not frag:
            return ""
        sx = float(getattr(e, "sx", 1.0) or 1.0)
        sy = float(getattr(e, "sy", 1.0) or sx)
        rot = float(getattr(e, "rot", 0.0) or 0.0)
        xf = "translate(%s %s)" % (_xf(vt.px(e.x)), _xf(vt.py(e.y)))
        if rot:
            xf += " rotate(%s)" % _xf(-rot)
        if sx != 1.0 or sy != 1.0:
            xf += " scale(%.4f %.4f)" % (sx, sy)
        return '<XGroup Transform="%s">%s</XGroup>' % (xf, frag)
    return ""


def _block_xml(blk, vt, cache):
    """块定义 → 块局部坐标的 XML 片段（同名块只算一次，供所有块参照复用）。"""
    if cache is None:
        return _build_block_xml(blk, vt, None)
    frag = cache.get(blk.name)
    if frag is None:
        frag = _build_block_xml(blk, vt, cache)
        cache[blk.name] = frag
    return frag


def _build_block_xml(blk, vt, cache):
    lvt = vt.local(blk.base)
    parts = []
    for sub in blk.entities:
        frag = entity_xml(sub, lvt, cache)
        if frag:
            parts.append(frag)
    return "".join(parts)


def grid_xml(vt, view, step_mm, color="#3A4046"):
    """栅格：按世界坐标铺线，与 grid_svg 同布局。"""
    x0, y0, x1, y1 = view
    if step_mm <= 0:
        return ""
    parts = []
    x = math.ceil(x0 / step_mm) * step_mm
    while x <= x1:
        parts.append(_xline(vt.px(x), vt.py(y0), vt.px(x), vt.py(y1), color, 0.6))
        x += step_mm
    y = math.ceil(y0 / step_mm) * step_mm
    while y <= y1:
        parts.append(_xline(vt.px(x0), vt.py(y), vt.px(x1), vt.py(y), color, 0.6))
        y += step_mm
    return "".join(parts)


def grips_xml(vt, entities, color="#E8B23D"):
    """选中实体的夹点小方块。"""
    half = max(2.0, vt.plen(22.0))
    parts = []
    for e in entities:
        for gx, gy in e.grips():
            sx, sy = vt.px(gx), vt.py(gy)
            parts.append('<XRect X="%s" Y="%s" W="%s" H="%s" Fill="%s" Stroke="#1B1D20" Width="0.80"/>'
                         % (_xf(sx - half), _xf(sy - half), _xf(half * 2), _xf(half * 2), color))
    return "".join(parts)


def _xalpha(color, alpha_hex):
    """给 #RRGGBB 加上 alpha，输出 #AARRGGBB（XML 侧统一 AARRGGBB，与 XAML 一致）。"""
    if not color.startswith('#'):
        return color
    return '#%s%s' % (alpha_hex, color[1:7])


def rubber_xml(vt, p1, p2, color="#E8B23D"):
    """窗口选择橡皮框（半透明填充 + 虚线边）。"""
    x0, y0, x1, y1 = geom_rect_norm(p1[0], p1[1], p2[0], p2[1])
    return ('<XRect X="%s" Y="%s" W="%s" H="%s" Fill="%s" Stroke="%s" Width="1.0" Dash="4.0,3.0"/>'
            % (_xf(vt.px(x0)), _xf(vt.py(y1)), _xf(vt.plen(x1 - x0)), _xf(vt.plen(y1 - y0)),
               _xalpha(color, '22'), color))


def crosshair_xml(vt, p, color="#C9D1D9"):
    """CAD 十字光标：贯穿视口的横竖线 + 中心空心方框（屏幕固定尺寸）。"""
    cx, cy = p
    x0, y0, x1, y1 = vt.x0, vt.y0, vt.x1, vt.y1
    zoom = vt.zoom if vt.zoom > 1e-6 else 1.0
    h = 6.0 / (zoom * vt.u)
    sw = 1.0 / zoom
    st = ' Stroke="%s" Width="%s"' % (color, _xf(sw))
    segs = [
        (vt.px(x0), vt.py(cy), vt.px(x1), vt.py(cy)),
        (vt.px(cx), vt.py(y0), vt.px(cx), vt.py(y1)),
        (vt.px(cx - h), vt.py(cy + h), vt.px(cx + h), vt.py(cy + h)),
        (vt.px(cx - h), vt.py(cy - h), vt.px(cx + h), vt.py(cy - h)),
        (vt.px(cx - h), vt.py(cy - h), vt.px(cx - h), vt.py(cy + h)),
        (vt.px(cx + h), vt.py(cy - h), vt.px(cx + h), vt.py(cy + h)),
    ]
    return "".join('<XLine X1="%s" Y1="%s" X2="%s" Y2="%s"%s/>'
                   % (_xf(a), _xf(b), _xf(c), _xf(d), st) for a, b, c, d in segs)


def to_xml(symbols, entities, view, grid=False, grid_step=0.0,
           selection=None, rubber=None, overlays=None, cursor=None, zoom=1.0,
           unit=None, cull=True, absolute=False):
    """符号表 + 实体表 → 一份 <XScene> 场景 XML（to_svg 的 XML 版）。

    首帧同样铺一块比视口大得多的底色矩形（画布 Background 属性在宿主里不生效，
    深色主题只能靠场景自己带底）。cull 语义与 to_svg 完全一致。
    """
    vt = ViewTransform(symbols, view, zoom=zoom, unit=unit, absolute=absolute)
    vw = vt.plen(view[2] - view[0])
    vh = vt.plen(view[3] - view[1])
    parts = ['<XScene Width="%s" Height="%s">' % (_xf(vw), _xf(vh)),
             '<XRect X="%s" Y="%s" W="%s" H="%s" Fill="%s"/>'
             % (_xf(vt.px(view[0]) - vw * 4), _xf(vt.py(view[3]) - vh * 4),
                _xf(vw * 9), _xf(vh * 9), CANVAS_BG)]
    if grid:
        parts.append(grid_xml(vt, view, grid_step))
    cache = {}
    margin = 24.0 / max(vt.u, 1e-12)
    lo_x, hi_x = view[0] - margin, view[2] + margin
    lo_y, hi_y = view[1] - margin, view[3] + margin
    for e in entities:
        if cull:
            b = e.bbox()
            if b[2] < lo_x or b[0] > hi_x or b[3] < lo_y or b[1] > hi_y:
                continue
        frag = entity_xml(e, vt, cache)
        if frag:
            parts.append(frag)
    if selection:
        parts.append(grips_xml(vt, selection))
    if rubber is not None:
        parts.append(rubber_xml(vt, rubber[0], rubber[1]))
    if overlays:
        parts.extend(overlays)
    if cursor is not None:
        parts.append(crosshair_xml(vt, cursor))
    parts.append("</XScene>")
    return "\n".join(parts)


def to_overlay_xml(symbols, view, unit=None, zoom=1.0, grid=False, grid_step=0.0,
                   selection=None, rubber=None, preview=None, cursor=None,
                   absolute=False):
    """动态叠加层（栅格 + 夹点 + 橡皮筋 + 预显 + 十字光标）：to_overlay_svg 的 XML 版。"""
    vt = ViewTransform(symbols, view, zoom=zoom, unit=unit, absolute=absolute)
    vw = vt.plen(view[2] - view[0])
    vh = vt.plen(view[3] - view[1])
    parts = ['<XScene Width="%s" Height="%s">' % (_xf(vw), _xf(vh))]
    if grid:
        parts.append(grid_xml(vt, view, grid_step))
    if preview is not None:
        frag = entity_xml(preview, vt, {})
        if frag:
            parts.append(frag)
    if selection:
        parts.append(grips_xml(vt, selection))
    if rubber is not None:
        parts.append(rubber_xml(vt, rubber[0], rubber[1]))
    if cursor is not None:
        parts.append(crosshair_xml(vt, cursor))
    parts.append("</XScene>")
    return "\n".join(parts)



def to_overlay_svg(symbols, view, unit=None, zoom=1.0, grid=False, grid_step=0.0,
                   selection=None, rubber=None, preview=None, cursor=None,
                   absolute=False):
    """动态叠加层：栅格 + 选中夹点 + 橡皮筋 + 预显图元 + 十字光标。

    这几样每帧都变，所以单独一层喂给画布的叠加场景（loadDwgOverlay）；
    底图（to_svg）只在图元真正变化时才重灌。坐标系与底图完全一致，
    画布用同一组 zoom/ox/oy 画，天然对齐。
    """
    vt = ViewTransform(symbols, view, zoom=zoom, unit=unit, absolute=absolute)
    vw = vt.plen(view[2] - view[0])
    vh = vt.plen(view[3] - view[1])
    parts = ['<?xml version="1.0" encoding="utf-8"?>',
             '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %.1f %.1f" '
             'width="%.1f" height="%.1f">' % (vw, vh, vw, vh)]
    if grid:
        parts.append(grid_svg(vt, view, grid_step))
    if preview is not None:
        frag = entity_svg(preview, vt, {})
        if frag:
            parts.append(frag)
    if selection:
        parts.append(grips_svg(vt, selection))
    if rubber is not None:
        parts.append(rubber_svg(vt, rubber[0], rubber[1]))
    if cursor is not None:
        parts.append(crosshair_svg(vt, cursor))
    parts.append("</svg>")
    return "\n".join(parts)
