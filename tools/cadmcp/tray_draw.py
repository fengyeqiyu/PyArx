# -*- coding: utf-8 -*-
"""把从 DWG 提取的桥架几何画成 SVG + XML 画布（XDwgCanvas.Data 内联 SVG）。

输入：G:\PyArx\tools\cadmcp\tray_out.json（extract_tray.py 在 CAD 内导出）
输出：G:\PyArx\docs\tray_plan.svg、tray_canvas.xml、tray_list.csv
"""
import csv
import json
import math
import os

SRC = r"G:\PyArx\tools\cadmcp\tray_out.json"
OUT_DIR = r"G:\PyArx\docs"
TRAY_W = 300.0        # 桥架宽度 300 mm（图纸标注 300 x 200）
PAD = 6000.0

data = json.load(open(SRC, encoding="utf-8"))
cntr = [l for l in data["lines"] if l["layer"] == "E-CABL-TRAY-CNTR"]
body = [l for l in data["lines"] if l["layer"] == "E-CABL-TRAY"]
labels = data["labels"]
fittings = data["fittings"]


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
             .replace('"', "&quot;"))


def mid(l):
    return ((l["start"][0] + l["end"][0]) / 2.0, (l["start"][1] + l["end"][1]) / 2.0)


def dist(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


# ---------------- 桥架分段 ---------------- 
runs = []
for i, l in enumerate(sorted(cntr, key=lambda x: (-x["length"], x["start"][1]), reverse=False), 1):
    m = mid(l)
    near = min(labels, key=lambda t: dist(m, t["pos"])) if labels else None
    spec, elev = "10KV电缆桥架 300 mm x 200 mm", None
    if near:
        spec = near["text"].rsplit(" ", 1)[0]
        try:
            elev = float(near["text"].rsplit(" ", 1)[-1])
        except ValueError:
            pass
    runs.append({
        "code": "TRAY-%02d" % i, "handle": l["handle"],
        "start": l["start"], "end": l["end"], "length": l["length"],
        "dir": l["dir"], "spec": spec, "elev": elev,
        "label_handle": near["handle"] if near else "",
    })

fits = []
for f in fittings:
    # 块定义内部的嵌套实体坐标恒为 (0,0)，不是模型空间实物，跳过
    if abs(f["pos"][0]) < 1e-6 and abs(f["pos"][1]) < 1e-6:
        continue
    kind = ("水平弯通" if "水平弯通" in f["block"] else
            ("上弯通" if "上弯通" in f["block"] else
             ("下弯通" if "下弯通" in f["block"] else "其他")))
    fits.append({"handle": f["handle"], "kind": kind, "block": f["block"],
                 "pos": f["pos"], "rot": round(math.degrees(f["rotation"]), 1)})

# ---------------- 画布尺寸 ---------------- 
xs, ys = [], []
for l in data["lines"]:
    xs += [l["start"][0], l["end"][0]]
    ys += [l["start"][1], l["end"][1]]
for f in fittings:
    xs.append(f["pos"][0]); ys.append(f["pos"][1])

x0, x1 = min(xs) - PAD, max(xs) + PAD
y0, y1 = min(ys) - PAD, max(ys) + PAD
W, H = x1 - x0, y1 - y0

MAXW, MAXH = 1500.0, 760.0
k = min(MAXW / W, MAXH / H)
VW, VH = W * k, H * k


def S(pt):
    """CAD 坐标(mm) → 画布像素（y 轴翻转）"""
    return ((pt[0] - x0) * k, (y1 - pt[1]) * k)


svg = ['<?xml version="1.0" encoding="utf-8"?>',
       '<svg xmlns="http://www.w3.org/2000/svg" width="%.0f" height="%.0f" viewBox="0 0 %.0f %.0f" '
       'font-family="Microsoft YaHei, sans-serif">' % (VW, VH + 56, VW, VH + 56),
       '<rect x="0" y="0" width="%.0f" height="%.0f" fill="#f7fafd"/>' % (VW, VH + 56),
       '<g transform="translate(0,56)">']

# 桥架本体（沿中心线两侧各 150 mm）
def band(l):
    ex, ey = l["end"][0] - l["start"][0], l["end"][1] - l["start"][1]
    L = math.hypot(ex, ey) or 1.0
    nx, ny = -ey / L, ex / L
    # 真实宽度 300 mm，缩放后不足 4 px 时按 4 px 画，保证屏幕上看得见
    hw = max(TRAY_W / 2.0, 3.0 / k)
    p1 = (l["start"][0] + nx * hw, l["start"][1] + ny * hw)
    p2 = (l["end"][0] + nx * hw, l["end"][1] + ny * hw)
    p3 = (l["end"][0] - nx * hw, l["end"][1] - ny * hw)
    p4 = (l["start"][0] - nx * hw, l["start"][1] - ny * hw)
    return [S(p) for p in (p1, p2, p3, p4)]


for l in cntr:
    pts = " ".join("%.1f,%.1f" % p for p in band(l))
    svg.append('<polygon points="%s" fill="#8ec0ea" fill-opacity="0.6" stroke="#1f5f9e" stroke-width="1"/>' % pts)

# 原始边线（还原图纸轮廓）
for l in body:
    a, b = S(l["start"]), S(l["end"])
    svg.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#b6cbde" stroke-width="0.6"/>'
               % (a[0], a[1], b[0], b[1]))

# 中心线
for l in cntr:
    a, b = S(l["start"]), S(l["end"])
    svg.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#17527f" stroke-width="1.2" '
               'stroke-dasharray="8 4"/>' % (a[0], a[1], b[0], b[1]))

# 弯通（CAD 旋转角是逆时针，画布 y 轴翻转后方向相反，故取负）
for f in fits:
    p = S(f["pos"])
    svg.append('<rect x="%.1f" y="%.1f" width="13" height="13" fill="#eda23c" stroke="#8a5a13" '
               'stroke-width="1" transform="rotate(%.1f %.1f %.1f)"/>'
               % (p[0] - 6.5, p[1] - 6.5, -f["rot"], p[0], p[1]))
    svg.append('<text x="%.1f" y="%.1f" font-size="11" fill="#8a5a13">%s</text>' % (p[0] + 9, p[1] - 9, f["kind"]))

# 分段编号 + 长度（贴边时改为右对齐，避免溢出画布）
for r in runs:
    p = S(mid(r))
    txt = "%s  L=%.2fm" % (r["code"], r["length"] / 1000.0)
    if r["dir"] == "V":
        ty = max(30.0, min(p[1], VH - 40.0))
        tx = p[0] + 14 if p[0] < VW - 170 else p[0] - 16
        svg.append('<text x="%.1f" y="%.1f" font-size="11" fill="#0f2f4a" '
                   'transform="rotate(-90 %.1f %.1f)">%s</text>' % (tx, ty, tx, ty, txt))
    else:
        overflow = p[0] + 6 + 7.0 * len(txt) > VW - 4
        if overflow:
            svg.append('<text x="%.1f" y="%.1f" font-size="11" fill="#0f2f4a" text-anchor="end">%s</text>'
                       % (p[0] - 6, p[1] - 8, txt))
        else:
            svg.append('<text x="%.1f" y="%.1f" font-size="11" fill="#0f2f4a">%s</text>' % (p[0] + 6, p[1] - 8, txt))

svg.append('</g>')
total = sum(r["length"] for r in runs) / 1000.0
svg.append('<text x="14" y="24" font-size="16" fill="#0f2f4a" font-weight="bold">'
           '10KV 电缆桥架平面（提取自 楼层平面-10KV电缆桥架.dwg）</text>')
svg.append('<text x="14" y="44" font-size="12" fill="#4f6f8a">'
           '桥架 %d 段，合计 %.2f m；弯通 %d 个（水平 %d / 上 %d / 下 %d）；'
           '桥架规格 %s</text>'
           % (len(runs), total, len(fits),
              sum(1 for f in fits if f["kind"] == "水平弯通"),
              sum(1 for f in fits if f["kind"] == "上弯通"),
              sum(1 for f in fits if f["kind"] == "下弯通"),
              runs[0]["spec"] if runs else ""))
svg.append('</svg>')
svg_text = "\n".join(svg)

os.makedirs(OUT_DIR, exist_ok=True)
svg_path = os.path.join(OUT_DIR, "tray_plan.svg")
open(svg_path, "w", encoding="utf-8").write(svg_text)

# ---------------- 清单 CSV ---------------- 
csv_path = os.path.join(OUT_DIR, "tray_list.csv")
with open(csv_path, "w", encoding="utf-8-sig", newline="") as f:
    w = csv.writer(f)
    w.writerow(["编号", "句柄", "起点X", "起点Y", "终点X", "终点Y", "长度m", "走向", "规格", "标高mm", "标注句柄"])
    for r in runs:
        w.writerow([r["code"], r["handle"], r["start"][0], r["start"][1], r["end"][0], r["end"][1],
                    round(r["length"] / 1000.0, 3), r["dir"], r["spec"], r["elev"], r["label_handle"]])

# ---------------- XML 画布（scene JSON：XDwgCanvas 原生按坐标绘制）----------------
# Data 里带 "layout" 走 setDwgScene；SVG 内联路径的自动 fit 在创建瞬间
# 画布尺寸为 0，实测 zoom=0（挤成一条），所以这里用 scene 格式。
scene_nodes = []
scene_links = []
for r in runs:
    a, b = S(r["start"]), S(r["end"])
    na, nb = r["code"] + "-A", r["code"] + "-B"
    for c, p in ((na, a), (nb, b)):
        scene_nodes.append({"code": c, "x": round(p[0] - 2, 1), "y": round(p[1] - 2, 1),
                            "w": 4.0, "h": 4.0, "depth": 0})
    scene_links.append({
        "from": na, "to": nb, "cable": "%s  L=%.2fm" % (r["code"], r["length"] / 1000.0),
        "x1": round(a[0], 1), "y1": round(a[1], 1),
        "x2": round(b[0], 1), "y2": round(b[1], 1),
        "vertical": 1 if r["dir"] == "V" else 0, "dashed": 0,
    })
for f in fits:
    p = S(f["pos"])
    scene_nodes.append({"code": f["kind"], "x": round(p[0] - 10, 1), "y": round(p[1] - 10, 1),
                        "w": 20.0, "h": 20.0, "depth": 0})

scene = {
    "dir": "h",
    "nodes": [{"code": n["code"], "name": n["code"], "upstream": "",
               "cable": "", "loadKw": 0, "voltage": "10KV", "length": 0,
               "role": "terminal", "collapsedCount": 0} for n in scene_nodes],
    "layout": {"width": int(VW), "height": int(VH) + 40,
               "nodes": scene_nodes, "links": scene_links},
}

xml = [
    '<?xml version="1.0" encoding="utf-8"?>',
    '<!-- 10KV 电缆桥架平面：XML 画布（XDwgCanvas，Data 为 scene JSON，按坐标绘制）',
    '     几何由 PyArx 从 楼层平面-10KV电缆桥架.dwg 提取（tools/cadmcp/tray_out.json） -->',
    '<XWindow Name="TrayPlan" Width="%d" Height="%d" Title="10KV 电缆桥架平面">'
    % (int(VW) + 40, int(VH) + 90),
    '  <XDwgCanvas Name="CV" Width="%d" Height="%d" Data="%s"/>'
    % (int(VW) + 20, int(VH) + 60, esc(json.dumps(scene, ensure_ascii=False).replace("\n", " ")),),
    '</XWindow>',
]
xml_path = os.path.join(OUT_DIR, "tray_canvas.xml")
open(xml_path, "w", encoding="utf-8").write("\n".join(xml))

print("runs=%d fittings=%d total=%.2fm  canvas=%.0fx%.0f" % (len(runs), len(fits), total, VW, VH))
print("svg %d bytes, xml %d bytes, csv %d bytes"
      % (os.path.getsize(svg_path), os.path.getsize(xml_path), os.path.getsize(csv_path)))
