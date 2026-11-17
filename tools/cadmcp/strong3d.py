# -*- coding: utf-8 -*-
"""楼层平面-强电.dwg：把桥架画成 3D（等轴测）SVG，并按规格统计长度。

输入：G:\PyArx\tools\cadmcp\strong_out.json（strong_extract.py 在 CAD 内导出）
输出：G:\PyArx\docs\strong3d_tray.svg、strong_tray_list.csv、strong_spec_summary.csv
"""
import collections
import csv
import json
import math
import os
import re

SRC = r"G:\PyArx\tools\cadmcp\strong_out.json"
OUT = r"G:\PyArx\docs"

d = json.load(open(SRC, encoding="utf-8"))
lines = d["lines"]
labels = d["labels"]
fits = d["fittings"]

cntr = [l for l in lines if l["layer"].endswith("CNTR")]
body = [l for l in lines if l["layer"].endswith("TRAY")]

SPEC_RE = re.compile(r"(\d+)\s*mm\s*x\s*(\d+)\s*mm\s*(\d+)?")


def parse_label(t):
    m = SPEC_RE.search(t)
    if not m:
        return None
    return {"w": int(m.group(1)), "h": int(m.group(2)),
            "z": float(m.group(3)) if m.group(3) else 0.0,
            "text": t}


parsed = [x for x in (parse_label(l["text"]) and dict(parse_label(l["text"]), pos=l["pos"], handle=l["handle"])
                      for l in labels) if x]

def mid(l):
    return ((l["start"][0] + l["end"][0]) / 2.0, (l["start"][1] + l["end"][1]) / 2.0)


def dist2(a, b):
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2


runs = []
# 1:1 最近匹配（一个标注只归属一段，按距离从近到远贪心）
pairs = []
for i, l in enumerate(cntr):
    m = mid(l)
    for j, t in enumerate(parsed):
        pairs.append((dist2(m, t["pos"]), i, j))
pairs.sort()
used_run, used_lab = {}, set()
for dd, i, j in pairs:
    if i in used_run or j in used_lab:
        continue
    used_run[i] = j
    used_lab.add(j)

for i, l in enumerate(cntr):
    near = parsed[used_run[i]] if i in used_run else None
    runs.append({
        "a": (l["start"][0], l["start"][1]), "b": (l["end"][0], l["end"][1]),
        "len": l["length"], "handle": l["handle"],
        "w": near["w"] if near else 200, "hh": near["h"] if near else 100,
        "z": near["z"] if near else 0.0,
        "spec": "%dx%d" % (near["w"], near["h"]) if near else "未标注",
        "label": near["handle"] if near else "",
    })

# 规格统计
spec_len = collections.defaultdict(lambda: {"len": 0.0, "n": 0, "z": set()})
for r in runs:
    spec_len[r["spec"]]["len"] += r["len"]
    spec_len[r["spec"]]["n"] += 1
    spec_len[r["spec"]]["z"].add(r["z"])

# ---------------- 等轴测投影 ----------------
COS30, SIN30 = math.cos(math.radians(30)), math.sin(math.radians(30))
ZEX = 12.0   # 竖向放大系数：全场 550 m，真实 200 mm 高只有 0.5 px，放 12 倍才看得出立体

def P3(x, y, z):
    return ((x - y) * COS30, (x + y) * SIN30 - z * ZEX)


pts = []
for r in runs:
    for (x, y) in (r["a"], r["b"]):
        pts.append(P3(x, y, r["z"]))
        pts.append(P3(x, y, r["z"] + r["hh"]))
for f in fits:
    pts.append(P3(f["pos"][0], f["pos"][1], 0))

minx = min(p[0] for p in pts); maxx = max(p[0] for p in pts)
miny = min(p[1] for p in pts); maxy = max(p[1] for p in pts)
W, H = maxx - minx, maxy - miny
CANVAS_W, CANVAS_H = 1600.0, 980.0
DW, DH = 1230.0, 860.0
OX, OY = 24.0, 86.0
k = min(DW / W, DH / H)
VW, VH = CANVAS_W, CANVAS_H

def S(p3):
    x, y = P3(*p3) if len(p3) == 3 else p3
    return ((x - minx) * k + OX, (y - miny) * k + OY)


def tray_box(r):
    """一段桥架的三维盒子：返回 (顶面, 左侧面, 右侧面) 多边形（已投影）"""
    ax, ay = r["a"]; bx, by = r["b"]
    dx, dy = bx - ax, by - ay
    L = math.hypot(dx, dy) or 1.0
    ux, uy = dx / L, dy / L
    nx, ny = -uy, ux
    hw = r["w"] / 2.0
    z0, z1 = r["z"], r["z"] + r["hh"]
    top = [(ax + nx * hw, ay + ny * hw, z1), (bx + nx * hw, by + ny * hw, z1),
           (bx - nx * hw, by - ny * hw, z1), (ax - nx * hw, ay - ny * hw, z1)]
    left = [(ax + nx * hw, ay + ny * hw, z0), (bx + nx * hw, by + ny * hw, z0),
            (bx + nx * hw, by + ny * hw, z1), (ax + nx * hw, ay + ny * hw, z1)]
    right = [(ax - nx * hw, ay - ny * hw, z0), (bx - nx * hw, by - ny * hw, z0),
             (bx - nx * hw, by - ny * hw, z1), (ax - nx * hw, ay - ny * hw, z1)]
    bottom = [(ax + nx * hw, ay + ny * hw, z0), (bx + nx * hw, by + ny * hw, z0),
              (bx - nx * hw, by - ny * hw, z0), (ax - nx * hw, ay - ny * hw, z0)]
    return top, left, right, bottom


svg = ['<?xml version="1.0" encoding="utf-8"?>',
       '<svg xmlns="http://www.w3.org/2000/svg" width="%.0f" height="%.0f" viewBox="0 0 %.0f %.0f" '
       'font-family="Microsoft YaHei, sans-serif">' % (CANVAS_W, CANVAS_H, CANVAS_W, CANVAS_H),
       '<rect x="0" y="0" width="%.0f" height="%.0f" fill="#f7fafd"/>' % (CANVAS_W, CANVAS_H)]

# 按投影深度排序，远的先画
order = sorted(runs, key=lambda r: (r["a"][0] + r["a"][1] + r["b"][0] + r["b"][1]))
for r in order:
    top, left, right, bottom = tray_box(r)
    poly = lambda ps: " ".join("%.1f,%.1f" % S(p) for p in ps)
    svg.append('<polygon points="%s" fill="#4c7fa6" fill-opacity="0.85" stroke="#2b5677" stroke-width="0.8"/>' % poly(bottom))
    svg.append('<polygon points="%s" fill="#6d9fc7" fill-opacity="0.95" stroke="#2b5677" stroke-width="0.8"/>' % poly(left))
    svg.append('<polygon points="%s" fill="#5c8db4" fill-opacity="0.95" stroke="#2b5677" stroke-width="0.8"/>' % poly(right))
    svg.append('<polygon points="%s" fill="#cfe4f7" stroke="#2b5677" stroke-width="0.9"/>' % poly(top))

# 配件（弯通/三通/四通/异径）画成小立方
def kind_of(name):
    for kk in ("水平四通", "水平三通", "水平弯通", "异径接头", "上弯通", "下弯通"):
        if kk in name:
            return kk
    return "配件"

VENT_COLOR = {"水平四通": ("#f0b357", "#d89434", "#b87a24"), "水平三通": ("#f2c777", "#e0a94f", "#bd8a34"),
              "水平弯通": ("#efa94a", "#d98b2e", "#b3711f"), "异径接头": ("#e08d68", "#c4714d", "#a05735"),
              "上弯通": ("#bba0e0", "#9d7fc4", "#8163a8"), "下弯通": ("#a68ccb", "#8a6fb0", "#6f5794"),
              "配件": ("#c2c8ce", "#a7aeb5", "#8d939a")}


def fit_spec(f, runs_all):
    """配件尺寸/标高取最近桥架段"""
    if not runs_all:
        return 300, 200, 0.0
    p = (f["pos"][0], f["pos"][1])
    best = min(runs_all, key=lambda r: min(dist2(p, r["a"]), dist2(p, r["b"])))
    return best["w"], best["hh"], best["z"]


real_fits = [f for f in fits if not (abs(f["pos"][0]) < 1e-6 and abs(f["pos"][1]) < 1e-6)]
for f in real_fits:
    w, hh, z = fit_spec(f, runs)
    s = max(220.0, w * 0.55)
    x, y = f["pos"][0], f["pos"][1]
    za, zb = z, z + hh * 0.9
    c = [(x - s, y - s), (x + s, y - s), (x + s, y + s), (x - s, y + s)]
    top = [(px, py, zb) for px, py in c]
    north = [(c[0][0], c[0][1], za), (c[1][0], c[1][1], za), (c[1][0], c[1][1], zb), (c[0][0], c[0][1], zb)]
    east = [(c[1][0], c[1][1], za), (c[2][0], c[2][1], za), (c[2][0], c[2][1], zb), (c[1][0], c[1][1], zb)]
    poly = lambda ps: " ".join("%.1f,%.1f" % S(p) for p in ps)
    ct, cn, ce = VENT_COLOR[kind_of(f["block"])]
    svg.append('<polygon points="%s" fill="%s" stroke="#5a3c18" stroke-width="0.7"/>' % (poly(north), cn))
    svg.append('<polygon points="%s" fill="%s" stroke="#5a3c18" stroke-width="0.7"/>' % (poly(east), ce))
    svg.append('<polygon points="%s" fill="%s" stroke="#5a3c18" stroke-width="0.8"/>' % (poly(top), ct))

legend = []
fitters = sum(1 for f in fits if not (abs(f["pos"][0]) < 1e-6 and abs(f["pos"][1]) < 1e-6))
total_len = sum(r["len"] for r in runs) / 1000.0
legend.append('<text x="16" y="30" font-size="17" fill="#0f2f4a" font-weight="bold">'
           '强电桥架三维示意（等轴测投影，源自 楼层平面-强电.dwg）</text>')
legend.append('<text x="16" y="50" font-size="12" fill="#4f6f8a">'
           '共 %d 段 / %.1f m；规格 %d 种；配件 %d 个（弯通/三通/四通/异径）；标高由最近标注确定</text>'
           % (len(runs), total_len, len(spec_len), fitters))
ty = 96
legend.append('<text x="%.0f" y="%d" font-size="13" fill="#0f2f4a" font-weight="bold">按规格统计</text>'
           % (1272.0, ty))
for spec, v in sorted(spec_len.items(), key=lambda kv: -kv[1]["len"]):
    ty += 23
    legend.append('<text x="%.0f" y="%d" font-size="12" fill="#31536f">%s：%.2f m（%d 段）</text>'
               % (1272.0, ty, spec, v["len"] / 1000.0, v["n"]))
ty += 36
legend.append('<text x="%.0f" y="%d" font-size="14" fill="#0f2f4a" font-weight="bold">合计 %.2f m</text>'
           % (1272.0, ty, total_len))

out = svg[:3] + legend + svg[3:] + ['</svg>']

svg_path = os.path.join(OUT, "strong3d_tray.svg")
open(svg_path, "w", encoding="utf-8").write("\n".join(svg))

# ---------------- CSV ----------------
list_path = os.path.join(OUT, "strong_tray_list.csv")
with open(list_path, "w", encoding="utf-8-sig", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["编号", "句柄", "起点X", "起点Y", "终点X", "终点Y", "长度m", "规格", "宽mm", "高mm", "标高mm", "标注句柄"])
    for i, r in enumerate(sorted(runs, key=lambda x: -x["len"]), 1):
        w.writerow(["STRONG-%03d" % i, r["handle"], r["a"][0], r["a"][1], r["b"][0], r["b"][1],
                    round(r["len"] / 1000.0, 3), r["spec"], r["w"], r["hh"], r["z"], r["label"]])

sum_path = os.path.join(OUT, "strong_spec_summary.csv")
with open(sum_path, "w", encoding="utf-8-sig", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["规格", "段数", "总长m", "标高集合"])
    for spec, v in sorted(spec_len.items(), key=lambda kv: -kv[1]["len"]):
        w.writerow([spec, v["n"], round(v["len"] / 1000.0, 3), " / ".join(str(int(z)) for z in sorted(v["z"]) if z)])

print("runs=%d total=%.1fm specs=%d" % (len(runs), sum(r["len"] for r in runs) / 1000.0, len(spec_len)))
for spec, v in sorted(spec_len.items(), key=lambda kv: -kv[1]["len"]):
    print("  %-8s %3d 段  %8.2f m  标高 %s" % (spec, v["n"], v["len"] / 1000.0,
                                              "/".join(str(int(z)) for z in sorted(v["z"]) if z)))
