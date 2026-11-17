# -*- coding: utf-8 -*-
"""抽样：按字高区间看文本内容，判断 5..500 区间是否为真实标注。"""
import json, os, tempfile, glob, random

f = glob.glob(os.path.join(os.path.join(tempfile.gettempdir(), "pyarx_scan"), "*.json"))[0]
at = json.load(open(f, encoding="utf-8"))["attrib"]
random.seed(1)

def band(lo, hi, n=10):
    sel = [a for a in at if lo <= float(a[2]) < hi]
    print("== band %s..%s  n=%d" % (lo, hi, len(sel)))
    for a in random.sample(sel, min(n, len(sel))):
        print("   h=%s x=%.0f y=%.0f t=%s layer=%s" % (a[2], a[0], a[1], a[6], a[8]))

band(5, 500)
band(100, 400)
band(500, 5000, 8)
