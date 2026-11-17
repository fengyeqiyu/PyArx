# -*- coding: utf-8 -*-
"""轴号：文字锚点 vs 圈心 的偏差定量。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

cr = d["cr"]           # 每 5 个一组 [cx,cy,r,a0,a1,ci,li]
circles = []
for i in range(0, len(cr) - 5, 5):
    cx, cy, r = cr[i], cr[i + 1], cr[i + 2]
    if 2000 < r < 20000 and -300000 < cy < -150000:
        circles.append((cx, cy, r))

tx = d["tx"]
digits = [t for t in tx if str(t[6]).strip().isdigit() and 250000 <= t[0] <= 600000
          and -300000 <= t[1] <= -150000]
print("circles:", len(circles), "digits:", len(digits))

for t in digits[:10]:
    best = None
    for (cx, cy, r) in circles:
        dd = ((cx - t[0]) ** 2 + (cy - t[1]) ** 2) ** 0.5
        if best is None or dd < best[0]:
            best = (dd, cx, cy, r)
    if not best:
        continue
    dd, cx, cy, r = best
    print("txt=%-3s anchor=(%.0f,%.0f) h=%.0f ha=%s va=%s | circle=(%.0f,%.0f) r=%.0f | dx=%.0f dy=%.0f (%.1f%%r)"
          % (t[6], t[0], t[1], t[2], t[4], t[8], cx, cy, r, t[0] - cx, t[1] - cy,
             100.0 * (t[1] - cy) / r))
