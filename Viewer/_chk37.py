# -*- coding: utf-8 -*-
"""madao111 / madao222 实例位置 + 块内标高文字条数。"""
import glob
import json
import os
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

for name in ("madao111", "madao222"):
    ins = [e for e in d["ins"] if e["k"] == name]
    blk = d["blk"].get(name, {})
    print("%s: ins=%d blk_tx=%d" % (name, len(ins), len(blk.get("tx") or [])))
    for e in ins[:6]:
        print("   pos=(%.0f,%.0f) sx=%.3f bbox=(%.0f,%.0f,%.0f,%.0f)"
              % (e["x"], e["y"], e["sx"], e["b"][0], e["b"][1], e["b"][2], e["b"][3]))
    for t in (blk.get("tx") or [])[:4]:
        print("   tx:", t)
