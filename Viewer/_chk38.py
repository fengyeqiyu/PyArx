# -*- coding: utf-8 -*-
"""块定义里的标高数字，经实例矩阵变换后的世界坐标，判断是否飞出版面。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

rx = re.compile(r"^\s*[+-]?\d+\.\d{3}\s*$")
sheet = (300000, -400000, 1900000, 900000)


def world(m, x, y):
    return (m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5])


cnt = 0
for e in d["ins"]:
    blk = d["blk"].get(e["k"]) or {}
    for t in (blk.get("tx") or []):
        if rx.match(str(t[6])):
            wx, wy = world(e["m"], t[0], t[1])
            ok = sheet[0] <= wx <= sheet[2] and sheet[1] <= wy <= sheet[3]
            cnt += 1
            if cnt <= 18:
                print("%-24s '%-7s' blk(%.0f,%.0f) -> world(%.0f,%.0f) %s"
                      % (e["k"][:24], t[6], t[0], t[1], wx, wy, "in-sheet" if ok else "OUT"))
print("total 标高文本实例:", cnt)
