# -*- utf-8 -*-
"""sys.json → MfcXml 绘图画布（loadDwgSvg），1:1 等比 / 整窗画布 / 可缩放平移"""
import json
import os
import time
import math

import PyMfcXml as M

SRC = r"C:\temp\sys.json"
OUT = r"G:\PyArx\docs"

# 图层配色（贴近 CAD 图面）
STYLE = {
    "L_S_WIRE": ("#C81E1E", 1.0),
    "DQ": ("#1E3CC8", 1.0),
    "SYS": ("#008080", 1.0),
    "L_S_EQPT": ("#966000", 1.0),
    "L_NOTE_C": ("#5A5A5A", 1.0),
    "L_WIRE": ("#AA28AA", 1.0),
    "TEL_MODI": ("#008C3C", 1.0),
    "C-Sheet-图框": ("#8C8C8C", 0.9),
    "C-Sheet-图框文字": ("#8C8C8C", 1.0),
    "C-Sheet-项目信息": ("#3C3C3C", 1.0),
    "0-签名": ("#5A5A5A", 1.0),
    "0": ("#141414", 1.0),
}
DEF = ("#141414", 1.0)


def style(l):
    return STYLE.get(l, DEF)


d = json.load(open(SRC, encoding="utf-8"))
X0, Y0, X1, Y1 = d["frame"]
SPANX, SPANY = X1 - X0, Y1 - Y0

import ctypes
_u = ctypes.windll.user32

SCR_W, SCR_H = _u.GetSystemMetrics(0), _u.GetSystemMetrics(1)
WIN_W, WIN_H = int(SCR_W * 0.93), int(SCR_H * 0.84)

win = M.Window("汇总图·母线干线系统图（A1 1:1）", WIN_W, WIN_H)
win.set_theme(False)
win.add_dwg_canvas(7200, 0, 0, WIN_W, WIN_H)
win.show()
time.sleep(0.6)
CW, CH = win.client_size()
print("client", CW, CH)

k = min(CW / SPANX, CH / SPANY)
imgW, imgH = SPANX * k, SPANY * k
px0 = (CW - imgW) / 2.0
py0 = (CH - imgH) / 2.0


def mp(x, y):
    return (px0 + (x - X0) * k, py0 + imgH - (y - Y0) * k)


parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'
         % (CW, CH, CW, CH),
         '<rect width="%d" height="%d" fill="#FFFFFF"/>' % (CW, CH)]

# 图面外标注（比例/范围）
parts.append('<text x="%.1f" y="%.1f" font-size="9" fill="#8C8C8C">'
             '母线干线系统图　A1 84100×59400 mm　1:%.0f</text>'
             % (px0, max(12, py0 - 4), 1.0 / k))

for ln in d["lines"]:
    col, sw = style(ln[4])
    a = mp(ln[0], ln[1])
    b = mp(ln[2], ln[3])
    parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f"/>'
                 % (a[0], a[1], b[0], b[1], col, sw))

for pl in d["plines"]:
    col, sw = style(pl["l"])
    pts = pl["p"]
    if pl["c"]:
        pts = pts + [pts[0]]
    for i in range(len(pts) - 1):
        a = mp(pts[i][0], pts[i][1])
        b = mp(pts[i + 1][0], pts[i + 1][1])
        parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f"/>'
                     % (a[0], a[1], b[0], b[1], col, sw))

for c in d["circles"]:
    col, sw = style(c[3])
    cx, cy = mp(c[0], c[1])
    r = c[2] * k
    if r < 0.6:
        continue
    seg = 20
    ring = []
    for i in range(seg + 1):
        a = 2 * math.pi * i / seg
        ring.append((cx + r * math.cos(a), cy - r * math.sin(a)))
    for i in range(seg):
        parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f"/>'
                     % (ring[i][0], ring[i][1], ring[i + 1][0], ring[i + 1][1], col, sw))

# 文字：真旋转（画布已支持 <text rotate>，绕锚点旋转）；CAD 逆时针 → SVG 顺时针取负
ANCH = {0: "start", 1: "middle", 2: "end", 3: "middle", 4: "middle", 5: "middle"}
n_txt = 0
for t in d["texts"]:
    col, _sw = style(t["l"])
    fs = max(4.0, t["h"] * k * 0.95)
    if fs > 42:
        fs = 42
    px, py = mp(t["x"], t["y"])
    anc = ANCH.get(int(t.get("ha", 0)), "start")
    deg = -math.degrees(t["r"])          # CAD: 逆时针正角 → SVG: 顺时针正角
    if abs(deg) < 0.5 or abs(abs(deg) - 360.0) < 0.5:
        deg = 0.0
    txt = t["t"].replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    parts.append('<text x="%.1f" y="%.1f" font-size="%.1f" fill="%s" text-anchor="%s" rotate="%.1f">%s</text>'
                 % (px, py, fs, col, anc, deg, txt))
    n_txt += 1

parts.append('</svg>')
svg = "".join(parts)
win.move(7200, 0, 0, CW, CH)
t0 = time.time()
win.load_dwg_svg(7200, svg)
print("svg %d chars load %.1fs texts=%d k=%.5f (1:%.0f)" % (len(svg), time.time() - t0, n_txt, k, 1.0 / k))
win.set_dwg_view(7200, 1.0, 0, 0)

with open(os.path.join(OUT, "sys_cad.svg"), "w", encoding="utf-8") as fp:
    fp.write(svg)

time.sleep(0.8)
win.save_control_png(7200, os.path.join(OUT, "sys_cad.png"))
print("png", os.path.getsize(os.path.join(OUT, "sys_cad.png")))

# 局部放大：取第一段母线的断路器组
zx = px0 + (470000 - X0) * k
zy = py0 + imgH - (-296000 - Y0) * k
zoom = 3.0
win.set_dwg_view(7200, zoom, int(zx - zx * zoom + 300), int(zy - zy * zoom + 200))
time.sleep(0.6)
win.save_control_png(7200, os.path.join(OUT, "sys_cad_zoom.png"))
print("zoom png", os.path.getsize(os.path.join(OUT, "sys_cad_zoom.png")))

KEEP = float(os.environ.get("SYS_KEEP", "0"))
if KEEP > 0:
    t_end = time.time() + KEEP
    while time.time() < t_end:
        M.pump(30)
else:
    win.close()
