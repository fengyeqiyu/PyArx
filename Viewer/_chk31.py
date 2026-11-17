# -*- coding: utf-8 -*-
"""看左侧 A 轴附近有哪些块参照、各自带多少属性/块内文字。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
print("ins sample:", json.dumps(d["ins"][0], ensure_ascii=False)[:300])
print("blk keys sample:", list(d["blk"].keys())[:3])
k0 = list(d["blk"].keys())[0]
print("blk item:", json.dumps({k: (str(v)[:80]) for k, v in d["blk"][k0].items()}, ensure_ascii=False))
