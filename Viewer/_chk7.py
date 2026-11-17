# -*- coding: utf-8 -*-
"""直接列出 scene html 里所有 "b":[...] 字段。"""
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
for m in re.finditer(r'"b":\[[^\]]*\]', s):
    print(m.group(0)[:100])
    break
for m in re.finditer(r'"cb":\[[^\]]*\]', s):
    print(m.group(0)[:100])
    break
