# -*- coding: utf-8 -*-
"""查上下两排轴网带里的全部文字，确认轴号字母到底有没有以 TEXT 存在。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DW = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
d = json.load(open(scan_load._cache_path(DW), encoding="utf-8"))
tx = d["texts"]

for tag, y0, y1 in (("upper", -222000, -186000), ("lower", -368000, -336000)):
    band = [t for t in tx if y0 <= t["y"] <= y1]
    print(tag, "n=", len(band))
    short = [t for t in band if len(t["t"].strip()) <= 3]
    print("   short:", [(t["t"], round(t["x"]), round(t["y"])) for t in short[:14]])
    if not short:
        for t in band[:6]:
            print("   ", t["t"][:22], round(t["x"]), round(t["y"]))

# 全图最短的文字在哪些区域
srt = sorted(tx, key=lambda t: len(t["t"].strip()))[:30]
print("shortest:", [(t["t"].strip(), round(t["x"]), round(t["y"])) for t in srt[:20]])
