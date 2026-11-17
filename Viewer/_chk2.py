# -*- coding: utf-8 -*-
"""检查生成的 scene html 是否带入属性文字（轴号圈 1~8 等）。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
print("html bytes", len(s))
m = re.search(r'"tx":\s*\[', s) or re.search(r'"tx":\[', s)
print("has tx", bool(m))
# tx 条目形如 [x,y,h,rot,ha,c,txt,va]，统计短数字串
for pat in (r',"1",0\]', r',"2",0\]', r',"3",0\]'):
    print(pat, s.count(pat))
c = s.count('"tx":[')
print('tx counts', c)
