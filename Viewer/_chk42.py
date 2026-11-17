# -*- coding: utf-8 -*-
"""全库重复文字检测：同坐标同文本是否被多处声明（顶层 vs 块定义）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

def k(t):
    return (round(t[0], 1), round(t[1], 1), str(t[6]).strip())

top = {k(t) for t in d["tx"]}
blkmap = {}
dup_in_blk = 0
for name, b in d["blk"].items():
    ks = [k(t) for t in (b.get("tx") or [])]
    if len(ks) != len(set(ks)):
        dup_in_blk += len(ks) - len(set(ks))
    for kk in ks:
        blkmap.setdefault(kk, set()).add(name)

multi = {kk: v for kk, v in blkmap.items() if len(v) > 1}
print("顶层 tx:", len(top), "块定义内重复:", dup_in_blk,
      "被多个块定义同时声明的文字:", len(multi))
for kk, v in list(multi.items())[:5]:
    print("  ", kk[2][:16], "in", list(v)[:3])

# 同一块被多个实例：该块文字会被画 N 次，这是正常的（不同位置）。
cnt = {}
for e in d["ins"]:
    cnt[e["k"]] = cnt.get(e["k"], 0) + 1
print("实例最多的块:", sorted(cnt.items(), key=lambda kv: -kv[1])[:5])
