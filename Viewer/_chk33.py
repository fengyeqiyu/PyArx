# -*- coding: utf-8 -*-
"""缓存 JSON 里，A 轴那几圈附近的所有 attrib（含空值）都列出来。"""
import glob
import json
import os

files = sorted(glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json")),
               key=os.path.getmtime)
print("cache files:", [os.path.basename(f) for f in files][-3:])
path = files[-1]
d = json.load(open(path, encoding="utf-8", errors="ignore"))
print("keys:", sorted(d.keys())[:12], "attrib:", len(d.get("attrib") or []))

near = [a for a in (d.get("attrib") or [])
        if 305000 <= a[0] <= 325000 and -320000 <= a[1] <= -250000]
print("nearby attrib:", len(near))
for a in sorted(near, key=lambda x: -x[1])[:25]:
    print("   x=%.0f y=%.0f h=%s rot=%s ha=%s va=%s txt=%r ly=%s"
          % (a[0], a[1], a[2], a[3], a[4], a[7], a[6], a[8]))
