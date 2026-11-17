# -*- coding: utf-8 -*-
"""递归找 atx 所在位置。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
js = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
print("blk sample def keys:", list(next(iter(js["blk"].values())).keys()))


def walk(node, path, out):
    if len(out) > 3:
        return
    if isinstance(node, dict):
        if "atx" in node:
            out.append((path, node.get("n"), node["atx"][:1]))
        for k2, v in node.items():
            walk(v, path + "/" + str(k2), out)
    elif isinstance(node, list):
        for i, v in enumerate(node[:600]):
            walk(v, path + "[%d]" % i, out)


out = []
walk(js, "", out)
for p, n, a in out:
    print(p[:120], "| n=", n, "| atx=", a)
axis = [x for x in out if "AXIS" in str(x[1])]
print("axis in first 3:", axis)
print("total atx in json:", s.count('"atx"'))
