# -*- coding: utf-8 -*-
"""按 CAD 图样绘制 2D 平面图（分幅 + 铺满客户区）。

铺满的关键是顺序：先建窗、show、取真实客户区尺寸，再按该尺寸生成 SVG。
否则 loadDwgSvg 内部 fitDwgCanvas 会等比缩放，SVG 比例与客户区比例不一致时
左右（或上下）必然留出画布底色带。
数据源 strong_2d.json（export_2d.py 从 DWG 原样导出，WCS mm）。"""
import ctypes
import json
import os
import re
import sys
import time

OUT = r"G:\PyArx\docs"
d = json.load(open(r"G:\PyArx\tools\cadmcp\strong_2d.json", encoding="utf-8"))

STYLE = {"E-CABL-TRAY": "#127E95",          # 主体：桥架边线
         "E-CABL-TRAY-CNTR": "#D93B3B",     # 中心线
         "E-CABL-TRAY-IDEN": "#8A5D00"}     # 标注（黄在浅底看不清，压深）
WID = {"E-CABL-TRAY": 1.1, "E-CABL-TRAY-CNTR": 1.7, "E-CABL-TRAY-IDEN": 1.0}
FIT = "#0B4E5E"
FRAME = "#9AA4B2"

pts = [l["a"] for l in d["lines"]] + [l["b"] for l in d["lines"]]
x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
y0, y1 = min(p[1] for p in pts), max(p[1] for p in pts)
spanX, spanY = x1 - x0, y1 - y0


def clip_x(ax, ay, bx, by, xl, xr):
    """把线段裁到 [xl, xr]，避免分幅边界外溢"""
    if ax > bx:
        ax, ay, bx, by = bx, by, ax, ay
    if bx <= xl or ax >= xr:
        return None
    t0, t1 = 0.0, 1.0
    dx = bx - ax
    if dx <= 0:
        return ((ax, ay), (bx, by))
    if ax < xl:
        t0 = (xl - ax) / dx
    if bx > xr:
        t1 = (xr - ax) / dx
    if t0 >= t1:
        return None
    return ((ax + dx * t0, ay + (by - ay) * t0),
            (ax + dx * t1, ay + (by - ay) * t1))


def pick_rows(CW, CH):
    """固定单幅：要求 1:1 等比，不切段。放大倍数由宽度约束，上下留白算图纸空白。"""
    return min(CW / spanX, CH / spanY), 1


def build_svg(CW, CH, N):
    """按客户区尺寸生成 SVG：图幅铺满 CW×CH，只留极窄的图幅框间隙"""
    k = min(CW * N / spanX, CH / (N * spanY))
    rowW = spanX / N * k
    rowH = spanY * k
    segSpan = spanX / N
    px0 = (CW - rowW) / 2.0                  # 宽度余量居中
    py0 = (CH - N * rowH) / (N + 1.0)        # 高度余量均分行间

    def mp(i, x, y):
        return (px0 + (x - x0 - i * segSpan) * k,
                py0 + i * (rowH + py0) + rowH - (y - y0) * k)

    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
             'viewBox="0 0 %d %d">' % (CW, CH, CW, CH),
             '<rect width="%d" height="%d" fill="#FFFFFF"/>' % (CW, CH)]

    for i in range(N):
        top = py0 + i * (rowH + py0)
        if N > 1:                     # 单幅不画图框，保持纯 CAD 图面
            parts.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="none" '
                         'stroke="%s" stroke-width="0.8"/>'
                         % (px0 - 3, top - 3, rowW + 6, rowH + 6, FRAME))
        sx0 = x0 + i * segSpan
        head = ("强电桥架平面图　x %.0f … %.0f mm　1:%.0f" % (sx0, sx0 + segSpan, 1.0 / k)
                if N == 1 else
                "图幅 %d/%d　x %.0f … %.0f mm　1:%.0f" % (i + 1, N, sx0, sx0 + segSpan, 1.0 / k))
        parts.append('<text x="%.1f" y="%.1f" font-size="9" fill="%s">%s</text>'
                     % (px0, top - 6, FRAME, head))

    for l in d["lines"]:
        col, sw = STYLE.get(l["lay"], "#127E95"), WID.get(l["lay"], 1.0)
        for i in range(N):
            xl = x0 + i * segSpan
            c = clip_x(l["a"][0], l["a"][1], l["b"][0], l["b"][1], xl, xl + segSpan)
            if c is None:
                continue
            p, q = mp(i, c[0][0], c[0][1]), mp(i, c[1][0], c[1][1])
            parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" '
                         'stroke-width="%s"/>' % (p[0], p[1], q[0], q[1], col, sw))

    for f in d["fits"]:
        for a, b in f["segs"]:
            for i in range(N):
                xl = x0 + i * segSpan
                c = clip_x(a[0], a[1], b[0], b[1], xl, xl + segSpan)
                if c is None:
                    continue
                p, q = mp(i, c[0][0], c[0][1]), mp(i, c[1][0], c[1][1])
                parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" '
                             'stroke-width="0.9"/>' % (p[0], p[1], q[0], q[1], FIT))

    total = 0
    for i in range(N):
        xl = x0 + i * segSpan
        raw = []
        for t in d["texts"]:
            tx = t["pos"][0]
            if tx < xl or tx >= xl + segSpan:
                continue
            m = re.search(r"(\d+)\s*mm\s*[xX×]\s*(\d+)\s*mm", t["text"])
            label = ("%s×%s" % (m.group(1), m.group(2))) if m else t["text"][:8]
            px, py = mp(i, tx, t["pos"][1])
            raw.append([label, px, py])

        clusters = []
        for label, px, py in sorted(raw, key=lambda r: (r[0], r[2], r[1])):
            for c in clusters:
                if (c["label"] == label and abs(py - c["cy"]) < 16
                        and abs(px - c["x1"]) < 110):
                    c["x1"] = px
                    c["n"] += 1
                    c["cy"] = py
                    break
            else:
                clusters.append({"label": label, "x0": px, "x1": px,
                                 "py": py, "cy": py, "n": 1})

        occupied = []
        for c in sorted(clusters, key=lambda c: -c["n"]):
            cx = (c["x0"] + c["x1"]) / 2.0
            txt = c["label"] if c["n"] == 1 else "%s (%d)" % (c["label"], c["n"])
            wpx = 7.0 * len(txt)
            box = (cx - wpx / 2, c["py"] - 5 - 9, cx + wpx / 2, c["py"] - 5 + 2)
            if any(not (box[2] < o[0] or box[0] > o[2] or box[3] < o[1] or box[1] > o[3])
                   for o in occupied):
                continue
            occupied.append(box)
            parts.append('<text x="%.1f" y="%.1f" font-size="7" text-anchor="middle" '
                         'fill="%s">%s</text>'
                         % (cx, c["py"] - 5, STYLE["E-CABL-TRAY-IDEN"], txt))
            total += 1

    parts.append("</svg>")
    print("客户区 %dx%d → %d 段分幅  k=%.5f px/mm (1:%.0f)  标注 %d 条 → 画 %d"
          % (CW, CH, N, k, 1.0 / k, len(d["texts"]), total))
    return "\n".join(parts), N


sys.path.insert(0, r"G:\PyArx\X64\Release")
os.add_dll_directory(r"G:\PyArx\X64\Release")
import PyMfcXml as M

_u = ctypes.windll.user32
SCR_W, SCR_H = _u.GetSystemMetrics(0), _u.GetSystemMetrics(1)

# 第一步：窗口直接开成屏幕大小 —— 单幅 1:1 时图只占中间一条，其余是白图纸，
# 窗口因此不必再按图纸比例裁窄
PAD = 10
WIN_W = int(SCR_W * 0.93)
WIN_H = int(SCR_H * 0.84)

N0 = 1    # 单幅 1:1，不分图
win = M.Window("强电桥架·二维图（1:1 单幅）", WIN_W, WIN_H)
win.set_theme(False)
win.add_dwg_canvas(7200, 0, 0, WIN_W, WIN_H)
win.show()
time.sleep(0.6)
CW, CH = win.client_size()
print("client", CW, CH)

# 第二步：按真实客户区出 SVG，画布回填到客户区 → 整个窗口都是画布
GH = CH                           # 画布已去掉 WS_HSCROLL，客户区高度就是绘制高度
svg, N = build_svg(CW, GH, N0)
win.move(7200, 0, 0, CW, GH)
t0 = time.time()
win.load_dwg_svg(7200, svg)
print("svg %d chars, load %.1fs" % (len(svg), time.time() - t0))
win.set_dwg_view(7200, 1.0, 0, 0)     # 1:1 映射：SVG 尺寸==客户区，跳过 fit 的 4px 留边
svg_path = os.path.join(OUT, "strong2d_cad.svg")
with open(svg_path, "w", encoding="utf-8") as fp:
    fp.write(svg)
time.sleep(0.8)
win.save_control_png(7200, os.path.join(OUT, "strong2d_cad.png"))
print("png", os.path.getsize(os.path.join(OUT, "strong2d_cad.png")))

zoom = 8.0
win.set_dwg_view(7200, zoom, int(1400 - 900 * zoom), int(CH / 2.0 - (CH / 2.0) * zoom))
time.sleep(0.6)
win.save_control_png(7200, os.path.join(OUT, "strong2d_cad_zoom.png"))
print("zoom png", os.path.getsize(os.path.join(OUT, "strong2d_cad_zoom.png")))

KEEP = float(os.environ.get("STRONG_KEEP", "0"))
if KEEP > 0:
    t_end = time.time() + KEEP
    while time.time() < t_end:
        M.pump(30)
else:
    win.close()
