# -*- coding: utf-8 -*-
"""看默认隐藏层的索引，以及 LY[0] 是谁。块内文字 li 恒为 0，若 0 被隐藏就全灭。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
ly = d["ly"]
print("lyh:", d.get("lyh"))
print("LY[:6]:", ly[:6])
print("LY count:", len(ly))
hid = set(d.get("lyh") or [])
print("0 hidden?", 0 in hid)
# 块内文字 li 值分布
lis = {}
for name, blk in list(d["blk"].items())[:400]:
    for t in (blk.get("tx") or []):
        lis[len(t)] = lis.get(len(t), 0) + 1
print("block tx field-length distribution:", lis)
