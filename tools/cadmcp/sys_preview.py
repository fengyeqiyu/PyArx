# -*- coding: utf-8 -*-
"""sys.json → PNG 预览（真实字体 SimSun，y 翻转按 CAD 习惯）"""
import json
import math
from PIL import Image, ImageDraw, ImageFont

SRC = r"C:\temp\sys.json"
DST = r"G:\PyArx\docs\sys_preview.png"
W = 2200

d = json.load(open(SRC, encoding="utf-8"))
x0, y0, x1, y1 = d["frame"]
k = W / (x1 - x0)
H = int((y1 - y0) * k)

# 图层配色（照 CAD 常见色）
COL = {
    "L_S_WIRE": (200, 30, 30),
    "DQ": (30, 60, 200),
    "SYS": (0, 120, 120),
    "L_S_EQPT": (150, 90, 0),
    "L_NOTE_C": (90, 90, 90),
    "L_WIRE": (170, 40, 170),
    "TEL_MODI": (0, 140, 60),
    "C-Sheet-项目信息": (60, 60, 60),
    "0": (20, 20, 20),
}

im = Image.new("RGB", (W, H), (255, 255, 255))
dr = ImageDraw.Draw(im)


def T(x, y):
    return ((x - x0) * k, (y1 - y) * k)


font_path = r"C:\Windows\Fonts\simsun.ttc"
_cache = {}


def font(sz):
    sz = max(6, int(round(sz)))
    if sz not in _cache:
        try:
            _cache[sz] = ImageFont.truetype(font_path, sz)
        except Exception:
            _cache[sz] = ImageFont.load_default()
    return _cache[sz]


for ln in d["lines"]:
    a = T(ln[0], ln[1])
    b = T(ln[2], ln[3])
    dr.line([a, b], fill=COL.get(ln[4], (20, 20, 20)), width=1)

for pl in d["plines"]:
    pts = [T(p[0], p[1]) for p in pl["p"]]
    if len(pts) >= 2:
        if pl["c"] and pts[0] != pts[-1]:
            pts.append(pts[0])
        dr.line(pts, fill=COL.get(pl["l"], (20, 20, 20)), width=1)

for c in d["circles"]:
    cx, cy = T(c[0], c[1])
    r = c[2] * k
    dr.ellipse([cx - r, cy - r, cx + r, cy + r], outline=COL.get(c[3], (20, 20, 20)))

for tx in d["texts"]:
    p = T(tx["x"], tx["y"])
    sz = tx["h"] * k * 1.05
    f = font(sz)
    txt = tx["t"]
    if len(txt) > 40:
        txt = txt[:40]
    try:
        dr.text(p, txt, fill=COL.get(tx["l"], (0, 0, 0)), font=f, anchor="ls")
    except Exception:
        pass

im.save(DST)
print("ok", W, H, "lines", len(d["lines"]), "texts", len(d["texts"]))
