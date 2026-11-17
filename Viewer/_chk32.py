# -*- coding: utf-8 -*-
"""定位 A 轴左侧那几圈属于哪个块，块内有没有文字/属性。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

PX, PY = 313866.0, -290000.0
hits = []
for e in d["ins"]:
    b = e.get("b") or [0, 0, 0, 0]
    if b[2] > b[0] and b[3] > b[1]:
        if b[0] - 6000 <= PX <= b[2] + 6000 and b[1] - 6000 <= PY <= b[3] + 6000:
            hits.append(e)
print("nearby inserts:", len(hits))
for e in hits[:20]:
    blk = d["blk"].get(e["k"], {})
    print("  %-40s pos=(%.0f,%.0f) sx=%.1f bbox=(%.0f,%.0f,%.0f,%.0f) tx=%d atx=%d"
          % (e["k"][:40], e["x"], e["y"], e["sx"], e["b"][0], e["b"][1], e["b"][2], e["b"][3],
             len(blk.get("tx") or []), len(blk.get("atx") or [])))
