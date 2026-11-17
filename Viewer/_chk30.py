# -*- coding: utf-8 -*-
"""左侧竖排轴号区：把所有单字符文字的坐标列出来，看哪些圈没字。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
lst = [t for t in d["tx"] if len(str(t[6]).strip()) == 1 and 250000 <= t[0] <= 420000
       and -330000 <= t[1] <= -150000]
lst.sort(key=lambda t: (t[0], -t[1]))
print("cols single-char:", len(lst))
for t in lst[:40]:
    print("  x=%.0f y=%.0f h=%.0f ha=%s va=%s '%s'" % (t[0], t[1], t[2], t[4], t[8], t[6]))
