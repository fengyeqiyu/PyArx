# -*- utf-8 -*-
"""render_box_raw —— 每台柜一张"图纸原图"（不重画）。

裁切框来自 boxframe.py 认出的柜体矩形（drb018_boxframes.json），
所以一台柜一张，不会把同列上下两台框进同一张。

渲染走 render_mfc.build_svg 的完整绘制链（块递归展开 + 坐标变换 +
ByLayer/ByBlock 取色 + 文字旋转），再用 MFCXaml 画布 loadDwgSvg 出 PNG。
"""

import ctypes
import json
import os
import re
import sys

sys.path.insert(0, r"G:\PyArx")
sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M

from pyarx_vision import frames as F
from pyarx_vision.render_mfc import build_svg

SCAN = r"C:\temp\scan.json"
FRAMES = r"G:\PyArx\docs\drb018_boxframes.json"
OUT = r"G:\PyArx\docs\boxes_raw"
CANVAS = 7200


def safe(s):
    s = re.sub(r'[\\/:*?"<>|]', "", s or "").strip()
    return s[:40] or "box"


def main():
    d = json.load(open(SCAN, encoding="utf-8"))
    bf = json.load(open(FRAMES, encoding="utf-8"))
    os.makedirs(OUT, exist_ok=True)
    fm = {f["no"]: f["frame"] for f in F.find_frames(d)}

    u = ctypes.windll.user32
    WW, WH = int(u.GetSystemMetrics(0) * 0.93), int(u.GetSystemMetrics(1) * 0.86)
    win = M.Window("柜体原图核对", WW, WH)
    win.set_theme(False)
    win.add_dwg_canvas(CANVAS, 0, 0, WW, WH)
    win.show()
    M.pump(900)
    CW, CH = win.client_size()
    win.move(CANVAS, 0, 0, CW, CH)
    M.pump(300)

    rows, seq = [], 0
    for b in bf["boxes"]:
        fr = fm.get(b["sheet"])
        if not fr:
            continue
        X0, Y0, X1, Y1 = fr
        # 柜框之外还要带上：左侧箱名、右侧回路号/断路器/电缆整列
        # （实测回路号 x 落在柜框右边界外约 140，电缆列再右 950）
        x0, x1 = b["x0"] - 600, b["x1"] + 1700
        y0, y1 = b["y0"] - 900, b["y1"] + 500
        frame = (max(X0, x0), max(Y0, y0), min(X1, x1), min(Y1, y1))
        if frame[2] - frame[0] < 800 or frame[3] - frame[1] < 800:
            continue
        svg, k, nt = build_svg(d, frame, CW, CH, margin=8)
        seq += 1
        nm = "%02d_%s" % (seq, safe(b["code"] or ("柜%d" % b["sheet"])))
        svg_p = os.path.join(OUT, nm + ".svg")
        open(svg_p, "w", encoding="utf-8").write(svg)
        win.load_dwg_svg(CANVAS, svg)
        M.pump(120)
        win.set_dwg_view(CANVAS, 1.0, 0, 0)
        M.pump(60)
        png = os.path.join(OUT, nm + ".png")
        win.save_control_png(CANVAS, png)
        kb = os.path.getsize(png) // 1024
        rows.append({"no": seq, "sheet": b["sheet"], "box": b["code"],
                     "x0": b["x0"], "y0": b["y0"], "x1": b["x1"], "y1": b["y1"],
                     "loopnos": b["loopnos"], "png": png, "svg": svg_p,
                     "texts": nt, "scale": "1:%d" % round(1.0 / k), "kb": kb})
        print("%-30s #%-2d 回路%-3d %-7s %dKB" % (nm, b["sheet"], b["loopnos"],
                                                 "1:%d" % round(1.0 / k), kb))

    json.dump(rows, open(OUT + "_index.json", "w", encoding="utf-8"),
              ensure_ascii=False, indent=1)
    win.close()
    print("共 %d 张 → %s" % (len(rows), OUT))


if __name__ == "__main__":
    main()
