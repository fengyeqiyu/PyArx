import math
# -*- coding: utf-8 -*-
"""用 MfcXml 的新控件 XView3D（原生三维画布）渲染 楼层平面-强电.dwg 的桥架三维图。

流程：strong_out.json → CNTR 段 + 1:1 最近标注匹配规格/标高 → view3DAppendBox 喂盒子
     → view3DFit → savePng 出图（默认视角 + 旋转视角各一张）
输出：G:\PyArx\docs\strong3d_native.png、strong3d_native_rot.png
"""
import json
import math
import os
import re
import sys
import time

SRC = r"G:\PyArx\tools\cadmcp\strong_out.json"
OUT = r"G:\PyArx\docs"

d = json.load(open(SRC, encoding="utf-8"))
lines, labels, fits = d["lines"], d["labels"], d["fittings"]
cntr = [l for l in lines if l["layer"].endswith("CNTR")]

SPEC_RE = re.compile(r"(\d+)\s*mm\s*x\s*(\d+)\s*mm\s*(\d+)?")


def parse_label(t):
    m = SPEC_RE.search(t)
    if not m:
        return None
    return {"w": int(m.group(1)), "h": int(m.group(2)),
            "z": float(m.group(3)) if m.group(3) else 0.0}


parsed = []
for l in labels:
    p = parse_label(l["text"])
    if p:
        p["pos"] = l["pos"]
        parsed.append(p)

def mid(l):
    return ((l["start"][0] + l["end"][0]) / 2.0, (l["start"][1] + l["end"][1]) / 2.0)


def dist2(a, b):
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2


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

runs = []
for i, l in enumerate(cntr):
    near = parsed[used_run[i]] if i in used_run else None
    runs.append({
        "a": l["start"], "b": l["end"], "len": l["length"],
        "w": near["w"] if near else 200, "hh": near["h"] if near else 100,
        "z": near["z"] if near else 5700.0,
    })

# 桥架网络包围盒：配件里有一批漂移坐标（嵌套块定义里的局部件），离网络很远的直接剔除，
# 否则包围盒被撑大、自动 fit 后整张图被压成一两个像素。
ax_ = [p for r in runs for p in (r["a"], r["b"])]
bx0, bx1 = min(p[0] for p in ax_), max(p[0] for p in ax_)
by0, by1 = min(p[1] for p in ax_), max(p[1] for p in ax_)
mx, my = (bx1 - bx0) * 0.03, (by1 - by0) * 0.06
def fit_xy(f):
    """配件在 WCS 的平面中心与真实尺寸：优先用 getGeomExtents 的真实包围盒"""
    bb = f.get("bbox")
    if bb:
        return ((bb[0][0] + bb[1][0]) / 2.0, (bb[0][1] + bb[1][1]) / 2.0,
                bb[1][0] - bb[0][0], bb[1][1] - bb[0][1])
    p = f["pos"]
    return (p[0], p[1], 0.0, 0.0)


real_fits = []
for f in fits:
    cx, cy, _, _ = fit_xy(f)
    if abs(cx) < 1e-6 and abs(cy) < 1e-6:
        continue
    if bx0 - mx <= cx <= bx1 + mx and by0 - my <= cy <= by1 + my:
        real_fits.append(f)
print("fits all=%d inside=%d dropped=%d" % (len(fits), len(real_fits), len(fits) - len(real_fits)))


# 比例参数：真实 1:1 时 ZEX=1、WMIN=0（557 m 跨度下桥架只有亚像素，肉眼看是一条线）
ZEX = float(os.environ.get("STRONG_ZEX", "10"))
WMIN = float(os.environ.get("STRONG_WMIN", "0"))   # 已改为断面整体放大，WMIN 不再参与
TAG = os.environ.get("STRONG_TAG", "")

def slab(ax, ay, bx, by, z0, z1, color):
    """一段槽式桥架 = 底板 + 两侧板（不画实心块，桥架是空心的 U 形槽）"""
    pass


def boxes_of(r):
    """槽式断面：底板 + 左右侧板。板厚按断面比例，斜段用轴对齐包络（近似）。"""
    ax, ay = r["a"][0], r["a"][1]
    bx, by = r["b"][0], r["b"][1]
    w = max(r["w"], 150.0) * ZEX            # 显示宽度
    h = max(r["hh"], 80.0) * ZEX            # 显示槽深
    t = max(w * 0.055, 60.0 * ZEX)          # 板厚
    z0 = r["z"] * ZEX
    along_x = abs(bx - ax) >= abs(by - ay)
    if along_x:
        x0, x1 = min(ax, bx) - t, max(ax, bx) + t   # 两端略出头，接缝不露空
        y0, y1 = min(ay, by) - w / 2.0, max(ay, by) + w / 2.0
        # 底板
        out = [(x0, y0, z0, x1, y1, z0 + t, 0x5E7F93)]
        # 左右侧板
        out.append((x0, y0, z0, x1, y0 + t, z0 + h, 0x6D9FC7))
        out.append((x0, y1 - t, z0, x1, y1, z0 + h, 0x6D9FC7))
    else:
        x0, x1 = min(ax, bx) - w / 2.0, max(ax, bx) + w / 2.0
        y0, y1 = min(ay, by) - t, max(ay, by) + t
        out = [(x0, y0, z0, x1, y1, z0 + t, 0x5E7F93)]
        out.append((x0, y0, z0, x0 + t, y1, z0 + h, 0x6D9FC7))
        out.append((x1 - t, y0, z0, x1, y1, z0 + h, 0x6D9FC7))
    return out


sys.path.insert(0, r"G:\PyArx\X64\Release")
os.add_dll_directory(r"G:\PyArx\X64\Release")
import PyMfcXml as M

import PyMfcXml as M

win = M.Window("强电桥架·三维画布（XView3D）", 1500, 940)
win.set_theme(False)
win.add_view3d(7100, 8, 8, 1484, 924)
win.view3d_set_style(7100, 0xF7FAFD, True, False)

def fit_color(name):
    if "四通" in name: return 0x8E7CC3
    if "三通" in name: return 0xC0504D
    if "异径" in name or "变径" in name: return 0x4F81BD
    if "弯" in name: return 0xE8913A
    if "终端" in name or "封" in name: return 0x7F8C97
    return 0x9AA6B2


t0 = time.time()
for r in runs:
    for bx_ in boxes_of(r):
        win.view3d_append_box(7100, *bx_)
skipped = []
for f in real_fits:
    cx, cy, wxy, hxy = fit_xy(f)
    best = min(runs, key=lambda r: min(dist2((cx, cy), r["a"]), dist2((cx, cy), r["b"])))
    # 远离任何桥架段的配件（图纸里散落的块）不画，否则看着像悬空
    dmin = math.sqrt(min(min(dist2((cx, cy), r["a"]), dist2((cx, cy), r["b"])) for r in runs))
    if dmin > max(3000.0, best["w"] * 4.0):
        skipped.append(f["handle"])
        continue
    # 显示尺寸与桥架显示宽度协调：位置用 WCS 真实中心，尺寸至少贴合桥架断面
    disp = max(best["w"], 150.0) * ZEX * 0.92
    # bbox 里若掺了引线/文字会异常大，超过断面 2.5 倍就按断面算
    wxy = min(wxy, disp * 2.5)
    hxy = min(hxy, disp * 2.5)
    sx = max(wxy, disp) / 2.0
    sy = max(hxy, disp) / 2.0
    z0, z1 = best["z"] * ZEX, (best["z"] + best["hh"]) * ZEX
    win.view3d_append_box(7100, cx - sx, cy - sy, z0, cx + sx, cy + sy, z1, fit_color(f["block"]))

win.show()
time.sleep(0.6)
win.view3d_fit(7100)
time.sleep(0.4)
print("camera", win.view3d_get_camera(7100))
win.save_control_png(7100, os.path.join(OUT, "strong3d_native%s.png" % TAG))

# 旋转视角：从另一个方向看，验证 yaw/pitch 生效
win.view3d_set_camera(7100, 125.0, 40.0, 0.86, 0, 0)
time.sleep(0.4)
win.save_control_png(7100, os.path.join(OUT, "strong3d_native%s_rot.png" % TAG))
# 默认不关窗口：留在屏幕上供手动拖动旋转 / 滚轮缩放，pump 消息持续响应
KEEP = float(os.environ.get("STRONG_KEEP", "150"))
if KEEP > 0:
    t_end = time.time() + KEEP
    while time.time() < t_end:
        M.pump(30)
else:
    win.close()

print("runs=%d fits=%d skipped=%d boxes=%.1fs" % (len(runs), len(real_fits), len(skipped), time.time() - t0))
for n in ("strong3d_native%s.png" % TAG, "strong3d_native%s_rot.png" % TAG):
    p = os.path.join(OUT, n)
    print(n, os.path.getsize(p) if os.path.exists(p) else "MISSING")
