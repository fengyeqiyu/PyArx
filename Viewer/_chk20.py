# -*- coding: utf-8 -*-
"""统计 scene 里单字符属性（轴号 1..9 / A,E,H）分布。"""
import json
import re
import collections

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>',
                         open(P, encoding="utf-8", errors="ignore").read(), re.S).group(1).strip())
tx = d["tx"]
if tx and isinstance(tx[0], (int, float)):
    tx = [tx[i:i + 9] for i in range(0, len(tx) - 8, 9)]
axis = [t for t in tx if str(t[6]).strip() in ("1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "E", "H")]
print("axis-like texts:", len(axis))
vals = collections.Counter(str(t[6]).strip() for t in axis)
print("values:", dict(vals))
xs = collections.Counter(int(t[0] / 20000) for t in axis)
print("x clusters(20k):", xs.most_common(6))
for t in axis[:14]:
    print("   x=%.0f y=%.0f h=%.1f ha=%s txt=%s" % (t[0], t[1], t[2], t[4], t[6]))
