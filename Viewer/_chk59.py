# -*- coding: utf-8 -*-
"""查扫描原始数据里：块定义内文字是否自身重复，以及块定义文字与属性值是否同位置。"""
import json, glob, os, collections

c = glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json"))[0]
D = json.load(open(c, encoding="utf-8"))
print("cache:", os.path.basename(c))
print("keys:", list(D.keys())[:20])

defs = D.get("block_defs") or {}
print("块定义 %d" % len(defs))

# 1) 块定义内文字自身同坐标重复
dupt = 0; samples = []
for name, bd in defs.items():
    g = (bd or {}).get("geom") or {}
    ts = g.get("texts") or []
    if len(ts) < 2:
        continue
    for i in range(len(ts)):
        for j in range(i + 1, len(ts)):
            a, b = ts[i], ts[j]
            if abs(a["x"] - b["x"]) <= 6 and abs(a["y"] - b["y"]) <= 6:
                dupt += 1
                if len(samples) < 10:
                    samples.append((name[:26], str(a.get("t"))[:14], str(b.get("t"))[:14], a["x"], a["y"]))
print("\n块定义内同位置文字对 %d" % dupt)
for s in samples:
    print("   %-26s '%s' vs '%s' @(%.0f,%.0f)" % s)

# 2) 块定义里是否还有 ATTDEF 类实体的痕迹
att = 0
for name, bd in defs.items():
    for k, v in (bd or {}).items():
        if isinstance(v, dict) and v.get("types"):
            pass
print("\n顶层 attrib 条数:", len(D.get("attrib") or ()))
print("顶层 texts 条数:", len((D.get("geom") or {}).get("texts") or ()))
