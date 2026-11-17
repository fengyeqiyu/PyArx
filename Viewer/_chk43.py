# -*- coding: utf-8 -*-
"""估算如果把块参照全部摊平，总线段/顶点规模有多大（决定是合批还是 tile 缓存）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

blk = d["blk"]

def size(name, depth=0, cache=None):
    if cache is None:
        cache = {}
    if name in cache:
        return cache[name]
    if depth > 10:
        return 0
    b = blk.get(name)
    if not b:
        return 0
    n = len(b.get("ln") or []) + len(b.get("cr") or []) * 24 + len(b.get("ar") or []) * 16
    for e in b.get("pl") or []:
        n += max(0, (len(e["p"]) // 2 - 1))
    for sub in b.get("ins") or []:
        n += size(sub.get("k"), depth + 1, cache)
    cache[name] = n
    return n

cache = {}
tot_ins = 0
instances = 0
for e in d["ins"]:
    tot_ins += size(e["k"], 0, cache)
    instances += 1
print("顶层块参照 %d 个，摊平后线段总量 ≈ %d" % (instances, tot_ins))
print("顶层 ln 线段 %d，pl %d，cr %d，ar %d，tx %d"
      % (len(d["ln"]) // 6, len(d["pl"]), len(d["cr"]) // 5, len(d["ar"]) // 7, len(d["tx"])))
print("块定义数 %d，有几何的 %d" % (len(blk), sum(1 for b in blk.values() if b.get("ln"))) )
