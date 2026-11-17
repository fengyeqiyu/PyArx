# -*- coding: utf-8 -*-
"""轴网带内的单字符文字检查（tx 里）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
tx = d["tx"]
band = [t for t in tx if str(t[6]).strip() and len(str(t[6]).strip()) <= 2
        and 250000 <= t[0] <= 600000 and -260000 <= t[1] <= -60000]
print("band single-char:", len(band))
vals = {}
for t in band:
    vals[str(t[6])] = vals.get(str(t[6]), 0) + 1
print("values:", dict(list(vals.items())[:20]))
for t in band[:12]:
    print("  x=%.0f y=%.0f h=%.1f ha=%s txt=%s" % (t[0], t[1], t[2], t[4], t[6]))
