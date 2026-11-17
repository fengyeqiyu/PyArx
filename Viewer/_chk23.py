# -*- coding: utf-8 -*-
"""统计 scene 里带 atx 的块参照与 atx 内容。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
tot = axi = 0
rows = []


def walk(entries, li, top=False):
    global tot, axi
    if li > 9 or not isinstance(entries, list):
        return
    for e in entries:
        if not isinstance(e, dict):
            continue
        if "atx" in e:
            tot += 1
            if "AXIS" in str(e.get("n", "")):
                axi += 1
                if len(rows) < 6:
                    rows.append([e.get("n"), e.get("atx")[:2]])
        if "ins" in e:
            walk(e["ins"], li + 1)


walk(d.get("ins") or [], 0, True)
for name, defn in (d.get("n") or {}).items() if isinstance(d.get("n"), dict) else []:
    pass
print("ins with atx(x):", tot, "AXIS:", axi)
for r in rows:
    print("  ", r)
print("sample top ins keys:", list((d.get("ins") or [{}])[0].keys()))
