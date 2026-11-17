# -*- coding: utf-8 -*-
"""统计 scene html 中 tx 里形如轴号的短文字条目。"""
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
i = s.find('"tx":[[')
seg = s[i:i + 600000]
# 条目形如 [x,y,h,rot,ha,ci,"文字",li,va]
ents = re.findall(r'\[\s*(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*)\s*,'
                  r'\s*(-?\d+\.?\d*)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*"([^"]*)"', seg)
print("entries scanned", len(ents))
axis = [e for e in ents if e[6].strip() in list("12345678ABCDEFGH")]
print("axis-like", len(axis))
for e in axis[:10]:
    print("  ", e)
