# -*- coding: utf-8 -*-
"""cr 组结构 + 轴号锚点/圈心垂直偏差。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
print("cr len", len(d["cr"]), "head", d["cr"][:10])

cr = d["cr"]
# 探测组步长：找 2000<r<20000 的位置
step = 5 if len(cr) % 5 == 0 else None   # [cx, cy, r, ci, li]
print("step", step)
if step:
    circles = []
    for i in range(0, len(cr) - step + 1, step):
        cx, cy, r = cr[i], cr[i + 1], cr[i + 2]
        if 1000 < r < 30000 and -300000 < cy < -150000:
            circles.append((cx, cy, r))
    print("circles in band", len(circles))
    tx = d["tx"]
    digs = [t for t in tx if str(t[6]).strip().isdigit() and 250000 <= t[0] <= 600000
            and -300000 <= t[1] <= -150000]
    for t in digs[:8]:
        best = min(circles, key=lambda c: (c[0] - t[0]) ** 2 + (c[1] - t[1]) ** 2) if circles else None
        if best:
            print("txt=%-3s anchor=(%.0f,%.0f) h=%.0f ha=%s va=%s | c=(%.0f,%.0f) r=%.0f | dx=%+.0f dy=%+.0f"
                  % (t[6], t[0], t[1], t[2], t[4], t[8], best[0], best[1], best[2],
                     t[0] - best[0], t[1] - best[1]))
