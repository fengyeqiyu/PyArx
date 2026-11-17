# -*- coding: utf-8 -*-
import re

s = open(r"G:\配电规划设计\web\101\scene.html", encoding="utf-8", errors="replace").read()
axiso = 0
with_atx = 0
samples = []
for m in re.finditer(r'"k":"([^"]*_AXISO[^"]*)",(.*?)}', s):
    axiso += 1
    seg = m.group(2)
    if '"atx":[[' in seg:
        with_atx += 1
        if len(samples) < 3:
            i = seg.find('"atx"')
            samples.append(seg[i:i + 90])
print("axiso_ins", axiso, "with_atx", with_atx)
for x in samples:
    print("  ", x)
