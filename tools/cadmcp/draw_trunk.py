# -*- utf-8 -*-
"""recog.json → MfcXml 配电画布：干线图重绘（最终版）
- 同一水平高度上的母线并排成一行（4# 与 2# 在原图同排，就按同排画，中间分组隔开）
- 备用回路用虚线支路 + 虚线断路器框 + 品红"备用"标记区分
- 每段按回路数均分出线：引线→断路器→容量/CT/kWh→编号与电缆竖排
"""
import json
import os
import time
import ctypes

import PyMfcXml as M

SRC = r"C:\temp\recog.json"
OUT = r"G:\PyArx\docs"
_u = ctypes.windll.user32
SCR_W, SCR_H = _u.GetSystemMetrics(0), _u.GetSystemMetrics(1)
WIN_W, WIN_H = int(SCR_W * 0.93), int(SCR_H * 0.84)

DESIGN = os.environ.get("DESIGN_IN", r"G:\PyArx\docs\trunk_design.json")
D = json.load(open(DESIGN, encoding="utf-8"))

# 设计模型 → 图面结构（渲染只认这份数据，换图不必改这里）
_by_bus = {}
for br in D["breakers"]:
    _by_bus.setdefault(br["bus"], []).append(br)
_buses = []
for bb in D["busbars"]:
    brs = sorted(_by_bus.get(bb["id"], []), key=lambda x: x["source_xy"][0])
    ld = bb.get("load", {})
    _buses.append({
        "spec": bb["spec"].get("raw", ""),
        "cap": "%dA" % bb["rated_a"],
        "station": bb["station"],
        "pe": ("Pe=%gkW" % ld["pe_kw"]) if "pe_kw" in ld else "",
        "cos": ("Cosφ=%g" % ld["cos_phi"]) if "cos_phi" in ld else "",
        "kx": " ".join(x for x in [("Kx=%g" % ld["kx"]) if "kx" in ld else "",
                                   ("Ijs=%gA" % ld["ijs_a"]) if "ijs_a" in ld else ""] if x),
        "top": bb["geom"]["y"], "x0": bb["geom"]["x0"], "x1": bb["geom"]["x1"],
        "count": len(brs),
        "loops": [{"cap": str(br["rated_a"]), "tag": br.get("circuit_no", ""),
                   "cable": br.get("cable", ""), "ct": br.get("ct", ""),
                   "meter": bool(br.get("metering")),
                   "spare": br.get("spare_note", "") or ("备用" if br.get("spare") else "")}
                  for br in brs],
    })
d = {"buses": _buses}
win = M.Window("干线系统图 · 四条母线（识别数据重绘）", WIN_W, WIN_H)
win.setTheme(False)
win.addPowerCanvas(7200, 0, 0, WIN_W, WIN_H)
win.show()
time.sleep(0.6)
CW, CH = win.clientSize()
print("client", CW, CH)

L0, R0 = 46, CW - 46
TOP, GAP, LEG = 54, 26, 40
C_BUS, C_BUS_ED = "#E3EDF7", "#1E3CC8"
C_LINE, C_TXT, C_DIM = "#3A3A3A", "#111111", "#5A5A5A"
C_SPARE, C_SPARE_LN = "#C0007A", "#C0007A"
BUS_H = 30
GROUP_X = 46                    # 同排两组母线之间的分组间隔

# ---- 行分组：同一水平高度的母线并排 ----
rows = []
for b in sorted(d["buses"], key=lambda r: (-r["top"], r["x0"])):
    for r in rows:
        if abs(r["top"] - b["top"]) < 1200:
            r["buses"].append(b)
            break
    else:
        rows.append({"top": b["top"], "buses": [b]})
for r in rows:
    r["buses"].sort(key=lambda b: b["x0"])
print("行:", [[b["spec"][:6] for b in r["buses"]] for r in rows])

ROWS = len(rows)
ROW_H = (CH - TOP - LEG - GAP * (ROWS - 1) - 12) / ROWS

p = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'
     % (CW, CH, CW, CH), '<rect width="%d" height="%d" fill="#FFFFFF"/>' % (CW, CH)]
p.append('<text x="%d" y="30" font-size="19" fill="%s" font-weight="700">'
         '母线干线系统图　101-1-EL-SYS-21</text>' % (L0, C_TXT))
n_loop = sum(b["count"] for r in rows for b in r["buses"])
n_spare = sum(1 for r in rows for b in r["buses"] for lo in b["loops"] if lo.get("spare"))
p.append('<text x="%d" y="30" font-size="12" fill="%s" text-anchor="end">'
         '%d 条母线　%d 个出线回路（虚线=备用 %d）　同排按站房分组</text>'
         % (R0, C_DIM, len(d["buses"]), n_loop, n_spare))

for ri, row in enumerate(rows):
    yTop = TOP + ri * (ROW_H + GAP)
    nbus = len(row["buses"])
    gapw = GROUP_X if nbus > 1 else 0
    tot_len = sum(b["x1"] - b["x0"] for b in row["buses"])
    avail = (R0 - L0) - gapw * (nbus - 1)

    off = L0
    for k, bus in enumerate(row["buses"]):
        w = avail * (bus["x1"] - bus["x0"]) / tot_len
        bx0, bx1 = off, off + w
        # 母线横梁
        p.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%d" fill="%s" stroke="%s" stroke-width="1.4"/>'
                 % (bx0, yTop, w, BUS_H, C_BUS, C_BUS_ED))
        p.append('<text x="%.1f" y="%.1f" font-size="14" fill="%s" font-weight="700">%s</text>'
                 % (bx0 + 9, yTop + BUS_H - 9, C_TXT, bus["spec"]))
        info = "　".join(x for x in [bus.get("station", ""), bus.get("cap", ""),
                                     bus.get("pe", ""), bus.get("kx", "")] if x)
        p.append('<text x="%.1f" y="%.1f" font-size="10.5" fill="%s" text-anchor="end">%s</text>'
                 % (bx1 - 9, yTop + BUS_H - 9, C_DIM, info))

        n = bus["count"] or 1
        cw = w / n
        yDrop = yTop + BUS_H + 46
        for j, lo in enumerate(bus["loops"]):
            cx = bx0 + cw * (j + 0.5)
            spare = bool(lo.get("spare"))
            lead = ('stroke="%s" stroke-width="1.6" stroke-dasharray="7 4"'
                    % C_SPARE_LN) if spare else ('stroke="%s" stroke-width="1.1"' % C_LINE)
            # 引线
            p.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" %s/>'
                     % (cx, yTop + BUS_H, cx, yDrop, lead))
            # 断路器（备用用虚线框）
            bw, bh = min(18, cw - 8), 10
            stroke = ('stroke="%s" stroke-width="1.3" stroke-dasharray="5 3"'
                      % C_SPARE_LN) if spare else ('stroke="%s" stroke-width="1.2"' % C_BUS_ED)
            fill = "#FFFFFF" if not spare else "#FDF2F8"
            p.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s" %s/>'
                     % (cx - bw / 2, yDrop, bw, bh, fill, stroke))
            # 容量
            col = C_SPARE if spare else C_TXT
            p.append('<text x="%.1f" y="%.1f" font-size="13" fill="%s" text-anchor="middle" font-weight="700">%sA</text>'
                     % (cx, yDrop + bh + 16, col, lo["cap"]))
            sub = "　".join(x for x in [lo.get("ct", ""), "kWh" if lo.get("meter") else ""] if x)
            if sub:
                p.append('<text x="%.1f" y="%.1f" font-size="9.5" fill="%s" text-anchor="middle">%s</text>'
                         % (cx, yDrop + bh + 29, C_DIM, sub))
            if spare:
                p.append('<text x="%.1f" y="%.1f" font-size="9" fill="%s" text-anchor="middle">备用</text>'
                         % (cx, yDrop + bh + 41, C_SPARE))
            # 编号 / 电缆竖排
            base = yDrop + bh + (46 if spare else 34)
            if lo.get("tag"):
                p.append('<text x="%.1f" y="%.1f" font-size="8" fill="%s" text-anchor="start" rotate="-90">%s</text>'
                         % (cx + 8, base + len(lo["tag"]) * 7.2, C_DIM, lo["tag"]))
            if lo.get("cable"):
                txt = lo["cable"].replace("-0.6/1KV", "")
                p.append('<text x="%.1f" y="%.1f" font-size="7.2" fill="#707070" text-anchor="start" rotate="-90">%s</text>'
                         % (cx - 8, base + len(txt) * 6.6, txt))

        off = bx1 + gapw
        # 组间分隔线
        if k < nbus - 1:
            p.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#B8B8B8" stroke-width="1" stroke-dasharray="4 4"/>'
                     % (bx1 + gapw / 2, yTop - 4, bx1 + gapw / 2, yTop + ROW_H - 10))

# ---- 图例 ----
yLeg = CH - 22
items = [("实线", C_LINE, False), ("备用（虚线）", C_SPARE_LN, True)]
x = L0
for label, col, dash in items:
    extra = ' stroke-dasharray="7 4"' if dash else ''
    p.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="1.6"%s/>'
             % (x, yLeg - 4, x + 26, yLeg - 4, col, extra))
    p.append('<text x="%d" y="%d" font-size="10" fill="%s">%s</text>' % (x + 32, yLeg, C_DIM, label))
    x += 32 + len(label) * 11 + 26

p.append('</svg>')
svg = "".join(p)
win.move(7200, 0, 0, CW, CH)
win.loadPowerSvg(7200, svg)
win.setPlanView(7200, 1.0, 0, 0)
open(os.path.join(OUT, "trunk_cad.svg"), "w", encoding="utf-8").write(svg)
time.sleep(0.8)
win.saveControlPng(7200, os.path.join(OUT, "trunk_cad.png"))
print("png", os.path.getsize(os.path.join(OUT, "trunk_cad.png")))
# 局部：第 3 行（4#|2# 同排）放大
win.setPlanView(7200, 1.9, -200, -420)
time.sleep(0.5)
win.saveControlPng(7200, os.path.join(OUT, "trunk_cad_zoom.png"))
win.setPlanView(7200, 1.0, 0, 0)
print("zoom ok")

KEEP = float(os.environ.get("TRUNK_KEEP", "0"))
if KEEP > 0:
    t_end = time.time() + KEEP
    while time.time() < t_end:
        M.pump(30)
