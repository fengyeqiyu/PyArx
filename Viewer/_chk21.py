# -*- coding: utf-8 -*-
"""在平面图轴网区域里找单字符文字（轴号），数据取自 tx。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())
tx = d["tx"]
print("tx total:", len(tx), "sample:", tx[0])
band = [t for t in tx if isinstance(t, list) and len(t) >= 7 and str(t[6]).strip()
        and len(str(t[6]).strip()) <= 2 and 300000 <= t[0] <= 560000 and -230000 <= t[1] <= -80000]
print("axis-band single-char:", len(band))
for t in band[:25]:
    print("   x=%.0f y=%.0f h=%.1f rot=%s ha=%s ci=%s txt=%s" % (t[0], t[1], t[2], t[3], t[4], t[5], t[6]))
