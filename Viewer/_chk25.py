# -*- coding: utf-8 -*-
"""统计非空 atx 并输出 AXISO 目标。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
js = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
stats = {"atx": 0, "nonempty": 0, "axis": 0}
samples = []


def walk(node):
    if isinstance(node, dict):
        if "atx" in node:
            stats["atx"] += 1
            if node["atx"]:
                stats["nonempty"] += 1
                if "AXIS" in str(node.get("n")):
                    stats["axis"] += 1
                    if len(samples) < 3:
                        samples.append((node.get("n"), node.get("x"), node.get("y"), node["atx"][:2]))
        for v in node.values():
            walk(v)
    elif isinstance(node, list):
        for v in node:
            walk(v)


walk(js)
print(stats)
for x in samples:
    print(x)
