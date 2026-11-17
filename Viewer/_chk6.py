# -*- coding: utf-8 -*-
"""打印 scene html 的视图范围 v 全部条目。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
m = re.search(r'"v":\[', s)
i = m.start() + len('"v":')
depth = 0
j = i
while j < len(s):
    if s[j] == "[":
        depth += 1
    elif s[j] == "]":
        depth -= 1
        if depth == 0:
            break
    j += 1
val = json.loads(s[i:j + 1])
for n, b in enumerate(val):
    w = b[2] - b[0]
    h = b[3] - b[1]
    print(n + 1, "w=%.0f h=%.0f ratio=%.2f" % (w, h, w / h if h else 0), b)
