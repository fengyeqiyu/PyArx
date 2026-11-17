# -*- coding: utf-8 -*-
import re

s = open(r"G:\配电规划设计\web\101\scene.html", encoding="utf-8", errors="replace").read()
n = 0
for m in re.finditer(r'"k":"([^"]*_AXISO[^"]*)",(.*?)}', s):
    n += 1
    if n <= 3:
        i = m.group(2).find('"atx"')
        print(m.group(1)[-14:], m.group(2)[i:i + 96])
print("axiso_ins", n)
