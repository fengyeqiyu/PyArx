# -*- coding: utf-8 -*-
"""DWG 解析结果 → cad2d 内部文档适配器。

分工（按既定架构）：
  · PyArxDwg 只负责解码 DWG，产出 ``Drawing`` 模型（实体/图层/块表）；
  · 本模块只做几何、颜色、线型、块参照到 cad2d 内部 ``db / ent`` 的映射；
  · 渲染仍走 cad2d 现有画布，本模块不碰解析、不碰绘制。

入口：
  · ``load_dwg(path) -> doc.Document``：端到端读文件并映射成内部文档；
  · ``build_document(drawing) -> doc.Document``：对已解码的 Drawing 做映射。
"""
import math
import os
import re
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.dirname(os.path.dirname(_HERE))  # G:\PyArx，PyArxDwg 包所在
if _ROOT not in sys.path:
    sys.path.insert(0, _ROOT)

import doc  # noqa: E402
import ent  # noqa: E402


# ACI 256 = ByLayer，内部用 0 表示随层。
def _color(value):
    try:
        return 0 if int(value) == 256 else int(value)
    except (TypeError, ValueError):
        return 0


# 解析侧线型名是中文/原名，内部线型表是英文；做一层别名，未知名落到实线。
_LTYPE_ALIAS = {
    "实线": "Continuous",
    "连续": "Continuous",
    "虚线": "Dashed",
    "点划线": "Center",
    "中心": "Center",
    "隐藏": "Hidden",
}


def _ltype(name):
    if not name or name == "ByLayer":
        return ""
    return _LTYPE_ALIAS.get(name, name)


def _xy(pt, default=0.0):
    if not pt:
        return (default, default)
    return (float(pt[0]), float(pt[1] if len(pt) > 1 else default))


# SHX 矢量形文件在后端没有对应字体，按字形风格替代为 TrueType；中文样式名保留，
# 由系统 TrueType 直接提供。
_SHX_FONT = {
    "romans": "Arial", "roman": "Arial", "romand": "Arial", "romanp": "Arial",
    "simplex": "Arial", "complex": "Arial", "italic": "Arial", "monotxt": "Consolas",
}


def _font(name):
    if not name:
        return "Microsoft YaHei"
    base = str(name).split("_")[0].strip().lower()
    if base in _SHX_FONT:
        return _SHX_FONT[base]
    if base == "arial":
        return "Arial"
    return str(name)


def _clean_mtext(text):
    """剥掉 MTEXT 内联格式码，只留可见正文。

    常见写法：``\\pt317.5,...,10159.99;正文``、``\\Ffont;``、``\\Hheight;``、
    ``\\Ccolor;``、``\\Aalign;``、``\\p`` 换行。无分号收尾的制表位串也一并清掉。
    """
    t = str(text or "")
    t = re.sub(r"\\[a-zA-Z]+[^;{}]*;", "", t)   # 带分号的格式码整段
    t = re.sub(r"\\[a-zA-Z]+[0-9.,-]*", "", t)  # 无分号残留（如 \pt 制表位串）
    t = t.replace("\\p", " ").replace("{", "").replace("}", "")
    return t.strip()


def _map_entity(g, layer, color, ltype):
    """单个解析实体 → 内部 ent；不支持的类型返回 None。"""
    t = g.get("type")
    geom = g.get("geometry") or {}
    kw = {"layer": layer, "color": color, "linetype": ltype}

    if t == "LINE":
        s = geom.get("start") or []
        e = geom.get("end") or []
        sx, sy = _xy(s)
        ex, ey = _xy(e)
        return ent.Line(sx, sy, ex, ey, **kw)

    if t == "CIRCLE":
        cx, cy = _xy(geom.get("center"))
        return ent.Circle(cx, cy, float(geom.get("radius") or 0.0), **kw)

    if t == "ARC":
        cx, cy = _xy(geom.get("center"))
        return ent.Arc(cx, cy, float(geom.get("radius") or 0.0),
                       float(geom.get("start_angle") or 0.0),
                       float(geom.get("end_angle") or 0.0), **kw)

    if t == "LWPOLYLINE":
        pts = [(float(v.get("x", 0.0)), float(v.get("y", 0.0)))
                for v in geom.get("vertices") or []]
        if len(pts) < 2:
            return None
        return ent.Polyline(pts, closed=bool(geom.get("closed", False)), **kw)

    if t in ("TEXT", "MTEXT"):
        ix, iy = _xy(geom.get("insert"))
        text = _clean_mtext(geom.get("text"))
        if not text:
            return None
        size = float(geom.get("height") or 0.0) or 200.0
        rot = 0.0
        ax = geom.get("x_axis") or geom.get("rotation")
        if ax:
            # x_axis 是方向向量：由它反推旋转角（度）。
            try:
                rot = math.degrees(math.atan2(float(ax[1]), float(ax[0])))
            except (TypeError, ValueError, IndexError):
                rot = float(geom.get("rotation") or 0.0)
        else:
            rot = float(geom.get("rotation") or 0.0)
        align = int(geom.get("attachment") or 7)
        style = _font(geom.get("style"))
        return ent.Text(ix, iy, text, size=size, style=style,
                        rot=rot, align=align, **kw)

    if t == "INSERT":
        ix, iy = _xy(geom.get("insert"))
        scale = geom.get("scale") or [1.0, 1.0, 1.0]
        sx = float(scale[0]) if len(scale) > 0 else 1.0
        sy = float(scale[1]) if len(scale) > 1 else 1.0
        rot = float(geom.get("rotation") or 0.0)
        return ent.Insert(str(geom.get("block") or ""), ix, iy,
                          sx=sx, sy=sy, rot=rot, **kw)

    if t == "ELLIPSE":
        # 内部无椭圆类：按长半轴近似成圆（major_axis 是半轴向量）。
        cx, cy = _xy(geom.get("center"))
        ax = geom.get("major_axis") or [1.0, 0.0, 0.0]
        r = math.hypot(float(ax[0]), float(ax[1] if len(ax) > 1 else 0.0)) or 1.0
        return ent.Circle(cx, cy, r, **kw)

    if t == "SPLINE":
        # 内部无样条类：用控制/拟合点连成折线近似。
        pts = [_xy(p) for p in (geom.get("control") or geom.get("fit") or [])]
        if len(pts) < 2:
            return None
        return ent.Polyline(pts, closed=bool(geom.get("closed", False)), **kw)

    if t == "HATCH":
        # 内部 HATCH 走 Python 扫描线；这里先取边界多边形做闭合多段线，
        # 避免剖面填充在大图纸上拖慢，边界轮廓先落图。
        verts = []
        for path in geom.get("paths") or []:
            for edge in path.get("edges") or []:
                s = edge.get("start")
                if s:
                    verts.append(_xy(s))
        if len(verts) < 3:
            return None
        return ent.Polyline(verts, closed=True, **kw)

    if t == "DIMENSION":
        p1 = _xy(geom.get("xline1"))
        p2 = _xy(geom.get("xline2"))
        dp = _xy(geom.get("def_point"))
        # off = 标注定位点在 p1→p2 法向上的投影距离（带符号）。
        dx, dy = p2[0] - p1[0], p2[1] - p1[1]
        n = math.hypot(dx, dy)
        off = 0.0
        if n > 1e-9:
            ux, uy = dx / n, dy / n
            nx, ny = -uy, ux
            off = (dp[0] - p1[0]) * nx + (dp[1] - p1[1]) * ny
        return ent.Dimension(p1, p2, off=off, **kw)

    # POLYLINE/VERTEX 需解析侧按 owned 关系配对（当前是独立记录），
    # 以及 3DFACE/SOLID/TRACE/POINT 等：内部暂未对应，跳过并计入 skipped。
    return None


def _bbox_of(entities):
    xs, ys = [], []
    for e in entities:
        try:
            x0, y0, x1, y1 = e.bbox()
            xs += [x0, x1]
            ys += [y0, y1]
        except Exception:
            continue
    if not xs:
        return (0.0, 0.0, 12000.0, 8000.0)
    return (min(xs), min(ys), max(xs), max(ys))


def build_document(drawing):
    """解码后的 Drawing → 内部 Document。"""
    docx = doc.Document(12000.0, 8000.0)

    # 图层（颜色是 ACI 索引，透传给内部；线宽解析侧未给，落默认 1.0）。
    for lay in drawing.layers:
        docx.layers.add(doc.Layer(
            lay.name, color=_color(lay.color), width=1.0,
            linetype=_ltype(lay.ltype)))

    # 块表：先登记块内实体（相对基点），块参照在主空间按 INSERT 摆位。
    for blk in drawing.blocks:
        if blk.name.startswith("*"):  # *Model_Space / *Paper_Space 系统块
            continue
        inner = []
        for g in blk.entities:
            e = _map_entity(g, g.get("layer", "0"), _color(g.get("color")),
                            _ltype(g.get("ltype")))
            if e is not None:
                inner.append(e)
        base = _xy(blk.base_point)
        docx.blocks.define(blk.name, inner, base=base)

    # 主空间实体。
    mapped, skipped = [], {}
    for g in drawing.entities:
        e = _map_entity(g, g.get("layer", "0"), _color(g.get("color")),
                        _ltype(g.get("ltype")))
        if e is None:
            skipped[g.get("type", "?")] = skipped.get(g.get("type", "?"), 0) + 1
            continue
        mapped.append(e)

    docx.add_many(mapped)

    # 幅面按实际实体包围盒定（DWG 坐标常为负、数值很大），留出边距。
    x0, y0, x1, y1 = _bbox_of(mapped)
    pad = max((x1 - x0), (y1 - y0), 1.0) * 0.1
    docx.width = max(x1 - x0 + pad * 2, 1.0)
    docx.height = max(y1 - y0 + pad * 2, 1.0)

    docx._dwg_meta = {
        "file": drawing.file,
        "version": drawing.version,
        "entity_total": len(mapped),
        "skipped": skipped,
        "warnings": list(drawing.warnings or []),
    }
    return docx


def load_dwg(path):
    """端到端：解码 DWG 文件并映射成内部 Document。"""
    import PyArxDwg as dwg
    drawing = dwg.read(path)
    return build_document(drawing)
