# -*- coding: utf-8 -*-
"""从 scene JSON 的 tx 里筛出轴号（单字符 1~8 / A~H，字高 300~900），打印坐标。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
m = re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S)
data = json.loads(m.group(1).strip())
tx = data.get("tx") or []
print("tx total", len(tx))
print("sample", tx[0])

hits = []
for t in tx:
    txt = str(t[6]).strip() if len(t) > 6 else ""
    h = float(t[2]) if len(t) > 2 else 0
    if len(txt) == 1 and txt in "123456789ABCDEFGH" and 200 <= h <= 900:
        hits.append((float(t[0]), float(t[1]), h, float(t[3]), t[4], txt))

hits.sort(key=lambda r: (r[5], -r[1]))
print("axis-like", len(hits))
for r in hits[:40]:
    print("x=%.0f y=%.0f h=%.0f rot=%.1f ha=%s txt=%s" % r)
