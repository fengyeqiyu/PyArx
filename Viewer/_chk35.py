# -*- coding: utf-8 -*-
"""缓存里所有像标高的短文本：含小数点、含 %% 转义、含 ±。"""
import glob
import json
import os
import re

f = sorted(glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json")),
           key=os.path.getmtime)[-1]
c = json.load(open(f, encoding="utf-8", errors="ignore"))
print("cache:", os.path.basename(f), {k: (len(v) if isinstance(v, list) else type(v).__name__)
                                      for k, v in c.items() if k in ("texts", "attrib", "mtexts", "counts")})

def scan(name, items, key):
    hits = []
    for it in items:
        v = str(it.get(key) if isinstance(it, dict) else it[6]).strip()
        if not v or len(v) > 12:
            continue
        if ("." in v and re.search(r"\d", v)) or "%%" in v or "±" in v:
            hits.append((v, it))
    print("%s 疑似标高 %d" % (name, len(hits)))
    seen = {}
    for v, it in hits:
        seen[v] = seen.get(v, 0) + 1
    for v, n in list(seen.items())[:20]:
        print("    %r x%d" % (v, n))
    for v, it in hits[:5]:
        if isinstance(it, dict):
            print("      at x=%.0f y=%.0f '%s'" % (it["x"], it["y"], v))
        else:
            print("      at x=%.0f y=%.0f '%s'" % (it[0], it[1], v))

scan("texts", c.get("texts") or [], "t")
scan("attrib", c.get("attrib") or [], None)
for k in ("mtext", "mtexts"):
    if c.get(k):
        scan(k, c[k], "t")
print("all keys:", sorted(c.keys()))
