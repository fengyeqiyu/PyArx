# -*- coding: utf-8 -*-
"""打印轴号条目的完整 9 字段，并为 1 号圈量像素偏差。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
for t in d["tx"]:
    if str(t[6]).strip() == "1" and 340000 <= t[0] <= 355000:
        print("entry:", t)
        break
print("ci/ly indexes?", sorted({(t[4], t[7], t[8]) for t in d["tx"]
                               if str(t[6]).strip().isdigit() and 250000 <= t[0] <= 600000
                               and -300000 <= t[1] <= -150000})[:6])
