# -*- coding: utf-8 -*-
"""统计 attrib 字高分布，定位未缩放的退化条目。"""
import json, os, tempfile, glob, collections

CACHE = os.path.join(tempfile.gettempdir(), "pyarx_scan")
f = glob.glob(os.path.join(CACHE, "*.json"))[0]
at = json.load(open(f, encoding="utf-8"))["attrib"]
print("attrib", len(at))
hist = collections.Counter()
for a in at:
    h = float(a[2])
    if h < 0.05:
        hist["h<0.05"] += 1
    elif h < 5:
        hist["0.05<=h<5"] += 1
    elif h < 500:
        hist["5<=h<500"] += 1
    elif h < 5000:
        hist["500<=h<5000"] += 1
    else:
        hist["h>=5000"] += 1
for k, v in hist.items():
    print(" ", k, v)
tiny = [a for a in at if float(a[2]) < 0.05]
print("tiny sample:")
for a in tiny[:8]:
    print("  ", a)
# 图层分布
c = collections.Counter(a[8] for a in tiny)
print("tiny layers", c.most_common(6))
# 文本重复度
c2 = collections.Counter(a[6] for a in tiny)
print("tiny texts", c2.most_common(6))
