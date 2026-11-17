# -*- coding: utf-8 -*-
"""查 scene 顶层结构 + 轴网区域单字符属性。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
body = re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1)
d = json.loads(body.strip())
print("keys:", list(d.keys()))
keys = [k for k in d.keys() if isinstance(d.get(k), list) and d.get(k) and isinstance(d[k][0], list)]
print("list-of-list keys:", keys)
