# -*- coding: utf-8 -*-
"""列出平面图区域内的小圆（轴号圈通常是 r≈200~400 的小圆）。"""
import json
import re
import collections

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>',
                         open(P, encoding="utf-8", errors="ignore").read(), re.S).group(1).strip())
cr = d["cr"]
if cr and isinstance(cr[0], (int, float)):
    cr = [cr[i:i + 5] for i in range(0, len(cr) - 4, 5)]
sel = [c for c in cr if 330000 <= c[0] <= 410000 and -340000 <= c[1] <= -240000]
print("circles in plan box:", len(sel))
rs = collections.Counter(round(c[2]) for c in sel)
print("radius hist:", rs.most_common(8))
small = [c for c in sel if c[2] < 600]
print("small:", len(small))
for c in small[:12]:
    print("   x=%.0f y=%.0f r=%s" % (c[0], c[1], c[2]))
