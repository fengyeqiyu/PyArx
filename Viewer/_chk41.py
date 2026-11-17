# -*- coding: utf-8 -*-
"""同一段文字在 scene 里出现几次、来自哪些块。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

TARGET = "报警阀间"
top = [t for t in d["tx"] if TARGET in str(t[6])]
print("顶层 tx 命中:", len(top), [t[6] for t in top[:3]])

owner = {}
for name, blk in d["blk"].items():
    for t in (blk.get("tx") or []):
        if TARGET in str(t[6]):
            owner.setdefault(name, []).append(t)
print("块定义命中:", {k: len(v) for k, v in owner.items()})

# 这些块各有几个实例
for k in owner:
    n = sum(1 for e in d["ins"] if e["k"] == k)
    print("  块 %-28s 定义内 %d 条, 实例 %d 个" % (k[:28], len(owner[k]), n))
    for e in [e for e in d["ins"] if e["k"] == k][:3]:
        print("     pos=(%.0f,%.0f) sx=%.2f sy=%.2f bbox=(%.0f,%.0f,%.0f,%.0f)"
              % (e["x"], e["y"], e["sx"], e["sy"], e["b"][0], e["b"][1], e["b"][2], e["b"][3]))
