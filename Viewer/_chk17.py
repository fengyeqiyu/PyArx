# -*- coding: utf-8 -*-
"""看 _AXISO 块定义里到底有什么（tx/atx/几何），以及实例数量。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>',
                         open(P, encoding="utf-8", errors="ignore").read(), re.S).group(1).strip())
blk = d["blk"]
for k, v in blk.items():
    if "AXIS" in k.upper():
        print("def", k, "keys", list(v.keys()))
        print("   tx", (v.get("tx") or [])[:3], "n=", len(v.get("tx") or []))
        print("   atx", (v.get("atx") or [])[:3])
        print("   ci/ar", len(v.get("ci") or []), len(v.get("ar") or []))
        n = sum(1 for i in d["ins"] if i.get("k") == k)
        print("   instances", n)
# 统计所有 def 的 tx 总数
print("defs with tx", sum(1 for v in blk.values() if v.get("tx")))
