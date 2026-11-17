# -*- coding: utf-8 -*-
"""检查 scene html 的视图范围 v / 主范围 b。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
for key in ("b", "cb", "v"):
    m = re.search(r'"%s":\[' % key, s)
    if not m:
        print(key, "absent")
        continue
    i = m.start() + len('"%s":' % key) - 1
    # 找配平
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
    raw = s[i:j + 1]
    try:
        val = json.loads(raw)
    except Exception as ex:
        print(key, "parse err", str(ex)[:40], raw[:80])
        continue
    print(key, "=", val)
