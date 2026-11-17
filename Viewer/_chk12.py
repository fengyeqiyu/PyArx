# -*- coding: utf-8 -*-
"""在 scan.json 的 attrib 里定位越界条目，打印全字段。"""
import json
import os
import tempfile
import glob

CACHE = os.path.join(tempfile.gettempdir(), "pyarx_scan")
files = glob.glob(os.path.join(CACHE, "*.json"))
print("cache files", files)
for f in files:
    data = json.load(open(f, encoding="utf-8"))
    if "attrib" not in data:
        continue
    at = data["attrib"]
    print("attrib", len(at))
    bad = [a for a in at if abs(float(a[0])) > 2.5e6]
    print("bad", len(bad))
    for a in bad[:10]:
        print("  ", a)
    # 半径近似：同一块的其它属性
    break
