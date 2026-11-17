# -*- coding: utf-8 -*-
"""查 scene 里 _AXISO 一类块定义是否带 ATTDEF 文本（atx）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
blk = d["blk"]
print("blk defs", len(blk))
hit = [(k, v) for k, v in blk.items() if "AXIS" in k.upper()]
print("axis defs", len(hit))
for k, v in hit[:6]:
    print(" ", k, "atx=", (v.get("atx") or [])[:4])
# 有多少块定义带 atx
n = sum(1 for v in blk.values() if v.get("atx"))
print("defs with atx", n)
ins = d["ins"]
print("ins", len(ins), "sample", ins[:2])
