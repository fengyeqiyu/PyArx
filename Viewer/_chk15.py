# -*- coding: utf-8 -*-
"""在 scene 里筛出平面图区域（x 340k~400k, y -330k~-250k）的短文字，看轴号是否就位。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
tx = d["tx"]
sel = [t for t in tx if 340000 <= float(t[0]) <= 400000 and -330000 <= float(t[1]) <= -250000]
print("in plan box:", len(sel))
short = [t for t in sel if len(str(t[6])) <= 3]
print("short:", len(short))
for t in short[:40]:
    print("  x=%.0f y=%.0f h=%s ha=%s txt=%s" % (float(t[0]), float(t[1]), t[2], t[4], t[6]))
