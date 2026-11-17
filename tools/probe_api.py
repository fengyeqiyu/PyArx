"""探查 PyArxCore 实际导出的名字（无头 CAD 内执行），用于定位兼容层缺项的真实归属。"""
import sys

import PyArxCore as C
from PyArxCore import Db, Ge, Ed, Gi, Rx, Ap

MODS = [("Core", C.Core)]
for name in ("Ed", "Ed.Core", "Db", "Db.Core"):
    parts = name.split(".")
    obj = C
    for p in parts:
        obj = getattr(obj, p, None)
        if obj is None:
            break
    if obj is not None:
        MODS.append((name, obj))

PATTERNS = ["cmd", "angto", "curdb", "xref", "alert", "getvar", "setvar", "trans",
            "coord", "vport", "arx"]

for label, mod in MODS:
    names = [n for n in dir(mod) if not n.startswith("__")]
    print("== %s (%d)" % (label, len(names)))
    for pat in PATTERNS:
        hits = [n for n in names if pat in n.lower()]
        if hits:
            print("  [%s] %s" % (pat, ",".join(sorted(hits)[:14])))
print("== done")
