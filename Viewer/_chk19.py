# -*- coding: utf-8 -*-
"""检查块定义文字（def.tx）与轴号块定义内容。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
txt = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', txt, re.S).group(1).strip())
blk = d["blk"]
withtx = [(k, v) for k, v in blk.items() if v.get("tx")]
kw = [k for k, _ in withtx]
for k in kw:
    if "_AXISO" in k or "AR-LAY" in k:
        v = blk[k]
        print("DEF", k, "tx=", v["tx"][:6], "ci=", len(v.get("ci") or []), "ln=", len(v.get("ln") or []))
print("defs with tx:", len(withtx), "of", len(blk))
