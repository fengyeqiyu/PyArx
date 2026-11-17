# -*- coding: utf-8 -*-
"""检查块定义文字是否把嵌套块文字重复收进来了。"""
import glob
import json
import os

f = sorted(glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json")),
           key=os.path.getmtime)[-1]
c = json.load(open(f, encoding="utf-8", errors="ignore"))
bd = c["block_defs"]

sample = "madao111"
g = bd[sample]["geom"]
print("块", sample, "texts", len(g.get("texts") or []), "ins", len(g.get("ins") or []))

# 子块里与父块文字同坐标的条数
def key(t):
    return (round(t["x"], 1), round(t["y"], 1), str(t.get("t") or "")[:20])

parent = {key(t) for t in (g.get("texts") or [])}
dup = 0
for sub in (g.get("ins") or []):
    n = sub.get("n")
    sg = (bd.get(n) or {}).get("geom") or {}
    stx = {key(t) for t in (sg.get("texts") or [])}
    ov = parent & stx
    if ov:
        dup += len(ov)
        if dup <= 40 or True:
            pass
print("父块文字中与子块文字坐标重合的:", dup)

# 全库统计：有多少块的 texts 里包含其子块 texts
tot = 0
for name, d in list(bd.items()):
    gg = d.get("geom") or {}
    p = {key(t) for t in (gg.get("texts") or [])}
    if not p:
        continue
    for sub in (gg.get("ins") or []):
        sg = (bd.get(sub.get("n")) or {}).get("geom") or {}
        tot += len(p & {key(t) for t in (sg.get("texts") or [])})
print("全库父子重合文字条数:", tot)
