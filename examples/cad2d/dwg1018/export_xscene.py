# -*- coding: utf-8 -*-
"""离线解码几何 -> MfcXml XScene 声明式 XML（loadDwgXml 直喂，不经 nanosvg）。
模型坐标：CAD y 向上 -> XScene y 向下，导出时 Y=-y。"""
import sys, math, json, io
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import render_svg

OUT = r"G:\PyArx\examples\cad2d\dwg1018\dwg_scene.xml"
REF_SCENE = r"G:\PyArx\viewer\web\scene.json"

elems, n = render_svg.collect_expanded()
COL = json.load(io.open(REF_SCENE, encoding="utf-8"))["c"]

box = render_svg._clip_box(elems, 0.05, 0.95)
xmin, ymin, xmax, ymax = box

def in_box(x, y):
    return xmin <= x <= xmax and ymin <= y <= ymax

def colhex(c):
    try:
        return COL[int(c)]
    except Exception:
        return "#111111"

def esc(t):
    return (t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
             .replace('"', "&quot;"))

def f(v):
    return f"{v:.2f}"

L = ["<XScene>"]
nl = nc = na = npl = nt = 0

for x1, y1, x2, y2, c in elems["line"]:
    if in_box(x1, y1) and in_box(x2, y2):
        L.append(f'<XLine X1="{f(x1)}" Y1="{f(-y1)}" X2="{f(x2)}" Y2="{f(-y2)}" Stroke="{colhex(c)}"/>')
        nl += 1

for cx, cy, r, c in elems["circle"]:
    if in_box(cx, cy):
        L.append(f'<XCircle Cx="{f(cx)}" Cy="{f(-cy)}" R="{f(r)}" Stroke="{colhex(c)}"/>')
        nc += 1

# 弧采样成折线（避开 XArc 翻转后角度方向问题）
def arc_pts(cx, cy, r, sa, ea):
    nseg = max(8, min(48, int(r / 15) + 8))
    out = []
    for i in range(nseg + 1):
        t = sa + (ea - sa) * i / nseg
        out.append((cx + r * math.cos(t), cy + r * math.sin(t)))
    return out

for cx, cy, r, sa, ea, c in elems["arc"]:
    if in_box(cx, cy):
        pts = arc_pts(cx, cy, r, sa, ea)
        if all(in_box(x, y) for x, y in pts):
            pts_s = " ".join(f"{f(x)} {f(-y)}" for x, y in pts)
            L.append(f'<XPolyline Points="{pts_s}" Stroke="{colhex(c)}"/>')
            na += 1

for pts, closed, c in elems["poly"]:
    if all(in_box(x, y) for x, y in pts):
        pts_s = " ".join(f"{f(x)} {f(-y)}" for x, y in pts)
        L.append(f'<XPolyline Points="{pts_s}" Closed="{1 if closed else 0}" Stroke="{colhex(c)}"/>')
        npl += 1

for pts, c in elems["spline"]:
    if len(pts) >= 2 and all(in_box(x, y) for x, y in pts):
        pts_s = " ".join(f"{f(x)} {f(-y)}" for x, y in pts)
        L.append(f'<XPolyline Points="{pts_s}" Stroke="{colhex(c)}"/>')
        npl += 1

for cx, cy, major, ratio, sa, ea, c in elems["ellipse"]:
    mx, my = major[0], major[1]
    mr = math.hypot(mx, my) or 1.0
    ux, uy = mx / mr, my / mr
    vx, vy = -uy, ux
    rmin = mr * ratio
    nseg = max(12, min(64, int(mr / 20)))
    pts = []
    for i in range(nseg + 1):
        t = sa + (ea - sa) * i / nseg
        dx = mr * math.cos(t); dy = rmin * math.sin(t)
        pts.append((cx + dx * ux + dy * vx, cy + dx * uy + dy * vy))
    if all(in_box(x, y) for x, y in pts):
        pts_s = " ".join(f"{f(x)} {f(-y)}" for x, y in pts)
        L.append(f'<XPolyline Points="{pts_s}" Stroke="{colhex(c)}"/>')
        npl += 1

for x, y, hgt, rot, t, c in elems["text"]:
    if in_box(x, y):
        t = t.replace(r"\P", "").strip()
        if not t:
            continue
        deg = -math.degrees(rot)
        L.append(f'<XText X="{f(x)}" Y="{f(-y)}" Size="{f(max(hgt, 1.0))}" '
                 f'Color="{colhex(c)}" Rotate="{f(deg)}">{esc(t)}</XText>')
        nt += 1

L.append("</XScene>")
io.open(OUT, "w", encoding="utf-8").write("\n".join(L))
print("line", nl, "circle", nc, "arc", na, "poly", npl, "text", nt)
print("b", box, "out", OUT)
