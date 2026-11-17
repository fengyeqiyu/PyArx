# -*- coding: utf-8 -*-
"""标高数值：scene.tx 与缓存 attrib 里三/四位小数的文本分布。"""
import glob
import json
import os
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
d = json.loads(re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S).group(1).strip())

pats = [
    ("±0.000", re.compile(r"^\u00b10\.000$")),
    ("3位小数", re.compile(r"^[-+±]?\d+\.\d{3}$")),
    ("含±", re.compile(r"[\u00b1]")),
]
for name, rx in pats:
    hit = [t for t in d["tx"] if rx.search(str(t[6]).strip())]
    print("tx %-8s %d" % (name, len(hit)))
    for t in hit[:6]:
        print("      x=%.0f y=%.0f h=%.0f '%s'" % (t[0], t[1], t[2], t[6]))

f = sorted(glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json")),
           key=os.path.getmtime)[-1]
c = json.load(open(f, encoding="utf-8", errors="ignore"))
at = c.get("attrib") or []
rx = re.compile(r"^[-+±]?\d+\.\d{3}$")
hit2 = [a for a in at if rx.search(str(a[6]).strip())]
print("attrib 三位小数 %d" % len(hit2))
for a in hit2[:6]:
    print("      x=%.0f y=%.0f h=%.0f '%s' ly=%s" % (a[0], a[1], a[2], a[6], a[8]))

# 文本实体（非属性）里的标高
tx2 = c.get("texts") or []
hit3 = [t for t in tx2 if rx.search(str(t.get("t") or "").strip())]
print("texts 三位小数 %d" % len(hit3))
for t in hit3[:6]:
    print("      x=%.0f y=%.0f h=%s '%s'" % (t["x"], t["y"], t.get("h"), t.get("t")))
