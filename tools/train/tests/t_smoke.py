# -*- coding: utf-8 -*-
"""冒烟：确认 run.py → PYARX_OUT → call_headless 读回的契约可用，并抓 Db/Ge 构造签名原文。"""

import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

sig = {}
for name in ("PyDbCircle", "PyDbLine", "PyDbPolyline", "PyDbText", "PyDbMText",
             "PyDbBlockReference", "PyDbHatch", "PyDbArc", "PyDbTable", "PyDbViewport",
             "PyDbTrans", "PyDbDatabase", "PyDbEntity", "PyDbObject",
             "PyDbBlockTableRecord", "PyDbLayerTableRecord", "PyDbCurve"):
    cls = getattr(P.Db, name, None)
    sig["Db." + name] = {"exists": cls is not None,
                         "hint": mlab.typeerror_hint(cls, "x") if cls is not None else "missing"}
for name in ("AcGeMatrix3d", "AcGeScale3d", "AcGePlane", "AcGeLineSeg3d", "AcGeCircArc3d",
             "AcGePoint3d", "AcGeVector3d", "AcGePoint2d"):
    cls = getattr(P.Ge, name, None)
    sig["Ge." + name] = {"exists": cls is not None,
                         "hint": mlab.typeerror_hint(cls, "x") if cls is not None else "missing"}

# Ed/Ap 侧：确认函数族与类的真实名字
ed = {
    "has_ssget": hasattr(P.Ed, "ssget"),
    "has_entget": hasattr(P.Ed, "entget"),
    "has_command": hasattr(P.Ed, "command"),
    "has_acedGetVar": hasattr(P.Ed, "acedGetVar"),
    "AdsName_hint": mlab.typeerror_hint(P.Ed.AdsName, "x"),
    "SelectionSet_hint": mlab.typeerror_hint(P.Ed.SelectionSet, "x"),
    "PyEdResult_members": [m for m in dir(P.Ed.PyEdResult) if not m.startswith("_")],
}
core = {m: [x for x in dir(getattr(P, m)) if "Ads" in x or "Engine" in x or "Cmd" in x][:20]
        for m in ("Core", "Rx", "Utility", "Enum", "Debug") if hasattr(P, m)}

out = mlab.out_path(r"C:\temp\smoke.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"sig": sig, "ed": ed, "core_scan": core}, f, ensure_ascii=False, indent=1)
print(json.dumps({"written": out, "n": len(sig), "ed": ed["has_ssget"]}))
