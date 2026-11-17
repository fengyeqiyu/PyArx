# -*- coding: utf-8 -*-
"""导出重点类的构造签名（pybind __init__ 文档）。"""
import json
import os

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_ctor.json"
R = {"db": {}, "ge": {}, "ed": {}, "done": False}

CLS = [
    "PyDbLine", "PyDbCircle", "PyDbArc", "PyDbPolyline", "PyDbText", "PyDbMText",
    "PyDbEllipse", "PyDbSpline", "PyDbPoint", "PyDbSolid", "PyDbBlockReference",
    "PyDbBlockTableRecord", "PyDbBlockTable", "PyDbLayerTable", "PyDbLayerTableRecord",
    "PyDbLinetypeTableRecord", "PyDbTextStyleTableRecord", "PyDbDimStyleTableRecord",
    "PyDbAlignedDimension", "PyDbRotatedDimension", "PyDbHatch", "PyDbRegion",
    "PyDb3dSolid", "PyDbWipeout", "PyDbRay", "PyDbXline", "PyDbAttribute",
    "PyDbAttributeDefinition", "PyDbTrans", "PyDbDatabase", "PyDbDictionary",
    "PyDbObject", "PyDbEntity", "PyDbCurve", "PyDbTable", "PyDbMLeader", "PyDbLeader",
    "PyDbViewportTableRecord", "PyDbXrecord",
]
GE = ["AcGePoint3d", "AcGeVector3d", "AcGePoint2d", "AcGeMatrix3d", "AcGeLineSeg3d",
      "AcGeCircArc3d", "AcGePlane", "AcGeVector2d", "AcGeScale3d"]


def d(o):
    init = getattr(o, "__init__", None)
    return ((getattr(init, "__doc__", "") or "").strip())[:700]


for n in CLS:
    c = getattr(P.Db, n, None)
    R["db"][n] = d(c) if c else "MISSING"
for n in GE:
    c = getattr(P.Ge, n, None)
    R["ge"][n] = d(c) if c else "MISSING"
for n in ("ssget", "acedSSGet", "entget", "entmake", "ssadd", "ssname"):
    f = getattr(P.Ed, n, None)
    R["ed"][n] = ((getattr(f, "__doc__", "") or "").strip())[:500] if f else "MISSING"

R["done"] = True
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(R, f, ensure_ascii=False, indent=1, default=str)
print("[PyArx] dump_ctor done")
