# -*- coding: utf-8 -*-
"""统计块参照包围盒异常（会拖慢 tile 渲染的坏框）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
ins = d["ins"]
bad = [e for e in ins if (e["b"][2] - e["b"][0]) > 1e7 or (e["b"][3] - e["b"][1]) > 1e7]
print("ins %d, bbox>1e7 %d" % (len(ins), len(bad)))
for e in bad[:8]:
    print("  %-32s b=%s m=%s" % (e["k"][:32], [round(v) for v in e["b"]], e["m"]))
# 屏幕尺寸视角：统计 bbox 面积超过图纸 b 面积 1e3 倍的实例
Bb = d["b"]
area = (Bb[2] - Bb[0]) * (Bb[3] - Bb[1])
huge = [e for e in ins if (e["b"][2] - e["b"][0]) * (e["b"][3] - e["b"][1]) > area * 1e3]
print("面积超图纸 1000 倍: %d" % len(huge))
