# -*- coding: utf-8 -*-
"""块定义内部的文字：找标高样式（x.xxx / ± / %%p）。"""
import glob
import json
import os
import re

f = sorted(glob.glob(os.path.join(os.environ["TEMP"], "pyarx_scan", "*.json")),
           key=os.path.getmtime)[-1]
c = json.load(open(f, encoding="utf-8", errors="ignore"))
bd = c.get("block_defs") or {}

rx = re.compile(r"^\s*[%]{0,2}[pP]?[\u00b1+-]?\d*\.\d{2,4}\s*$")
found = {}
for name, d in bd.items():
    g = d.get("geom") or {}
    for t in (g.get("texts") or ()):
        v = str(t.get("t") or "").strip()
        if v and (rx.match(v) or "0.000" in v or "%%p" in v.lower()):
            found.setdefault(v, []).append((name, t.get("x"), t.get("y"), t.get("h")))
print("block_defs 标高样式文字:", len(found))
for v, lst in list(found.items())[:15]:
    print("  %r  in %d defs, e.g. %s" % (v, len(lst), lst[0]))
print("blk_with_text:", sum(1 for d in bd.values() if (d.get("geom") or {}).get("texts")))
