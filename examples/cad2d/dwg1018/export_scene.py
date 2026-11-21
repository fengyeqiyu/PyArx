# -*- coding: utf-8 -*-
"""离线解码几何 -> 既有 viewer 扁平 scene schema（scene_offline.json）。"""
import sys, math, json, io
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import render_svg

OUT = r"G:\PyArx\viewer\web\scene_offline.json"
REF_SCENE = r"G:\PyArx\viewer\web\scene.json"

elems, n = render_svg.collect()

# 颜色表：复用既有 scene 的 257 项 ACI 配色
col = json.load(io.open(REF_SCENE, encoding="utf-8"))["c"]

# 分位裁剪主图框（同 SVG 口径）
box = render_svg._clip_box(elems)
xmin, ymin, xmax, ymax = box

def in_box(x, y):
    return xmin <= x <= xmax and ymin <= y <= ymax

ln, cr, ar, pl, tx = [], [], [], [], []

for x1, y1, x2, y2, c in elems["line"]:
    if in_box(x1, y1) and in_box(x2, y2):
        ln += [round(x1, 3), round(y1, 3), round(x2, 3), round(y2, 3), c, 0]

for cx, cy, r, c in elems["circle"]:
    if in_box(cx, cy):
        cr += [round(cx, 3), round(cy, 3), round(r, 3), c, 0]

for cx, cy, r, sa, ea, c in elems["arc"]:
    if in_box(cx, cy):
        ar += [round(cx, 3), round(cy, 3), round(r, 3), round(sa, 5), round(ea, 5), c, 0]

for pts, closed, c in elems["poly"]:
    if all(in_box(x, y) for x, y in pts):
        p = [round(v, 3) for xy in pts for v in xy]
        xs = [q[0] for q in pts]; ys = [q[1] for q in pts]
        pl.append({"p": p, "z": 1 if closed else 0, "c": c,
                   "b": [min(xs), min(ys), max(xs), max(ys)], "li": 0})

# 样条折线近似成多段线
for pts, c in elems["spline"]:
    if all(in_box(x, y) for x, y in pts) and len(pts) >= 2:
        p = [round(v, 3) for xy in pts for v in xy]
        xs = [q[0] for q in pts]; ys = [q[1] for q in pts]
        pl.append({"p": p, "z": 0, "c": c,
                   "b": [min(xs), min(ys), max(xs), max(ys)], "li": 0})

# 椭圆采样成多段线
for cx, cy, major, ratio, sa, ea, c in elems["ellipse"]:
    if not in_box(cx, cy):
        continue
    mx, my = major[0], major[1]
    mr = math.hypot(mx, my) or 1.0
    ux, uy = mx / mr, my / mr
    vx, vy = -uy, ux
    rmin = mr * ratio
    pts = []
    nseg = max(12, min(64, int(mr / 20)))
    for i in range(nseg + 1):
        t = sa + (ea - sa) * i / nseg
        dx = mr * math.cos(t); dy = rmin * math.sin(t)
        pts.append((cx + dx * ux + dy * vx, cy + dx * uy + dy * vy))
    if all(in_box(x, y) for x, y in pts):
        p = [round(v, 3) for xy in pts for v in xy]
        xs = [q[0] for q in pts]; ys = [q[1] for q in pts]
        pl.append({"p": p, "z": 0, "c": c,
                   "b": [min(xs), min(ys), max(xs), max(ys)], "li": 0})

for x, y, hgt, rot, t, c in elems["text"]:
    if in_box(x, y):
        t = t.replace(r"\P", "").strip()
        if not t:
            continue
        tx.append([round(x, 3), round(y, 3), round(max(hgt, 1.0), 2),
                   round(rot, 5), 0, c, t, 0, 0])

nb = [round(xmin, 3), round(ymin, 3), round(xmax, 3), round(ymax, 3)]
scene = {
    "c": col,
    "ln": ln, "cr": cr, "ar": ar, "pl": pl, "tx": tx,
    "blk": {}, "ins": [], "tb": [],
    "b": nb, "cb": nb, "v": [nb],
    "ly": ["0"], "lyc": [len(ln) // 6 + len(cr) // 5 + len(ar) // 7 + len(pl) + len(tx)],
    "lyh": [],
    "f": "中试车间扩建-工艺配电及系统2026.10.12_t3.dwg（离线解码）",
    "n": len(ln) // 6 + len(cr) // 5 + len(ar) // 7 + len(pl) + len(tx),
}
json.dump(scene, io.open(OUT, "w", encoding="utf-8"), ensure_ascii=False, separators=(",", ":"))
print("ln", len(ln) // 6, "cr", len(cr) // 5, "ar", len(ar) // 7,
      "pl", len(pl), "tx", len(tx), "n", scene["n"], "b", nb)
print("out", OUT)
