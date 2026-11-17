# -*- coding: utf-8 -*-
"""从 scene 的 JSON 脚本块里权威读取 b / cb / v。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
m = re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S)
if not m:
    raise SystemExit("scene block not found")
raw = m.group(1).strip()
data = json.loads(raw)
print("keys:", sorted(data.keys())[:20])
for k in ("b", "cb", "v"):
    val = data.get(k)
    if val is None:
        print(k, "MISSING")
        continue
    if k == "v":
        print("v count", len(val))
        for n, b in enumerate(val):
            print("  ", n + 1, [round(x) for x in b], "w=%.0f h=%.0f" % (b[2] - b[0], b[3] - b[1]))
    else:
        print(k, [round(x) for x in val], "w=%.0f h=%.0f" % (val[2] - val[0], val[3] - val[1]))
print("tx count", len(data.get("tx") or []))
