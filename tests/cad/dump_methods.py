# -*- coding: utf-8 -*-
"""导出关键方法的完整签名（用于写可运行 sample）。"""
import json

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_methods.json"
Q = {
    "Db": {
        "PyDbSymbolTable": ["add", "getAt", "getIdAt", "records", "items", "values", "keys", "toDict", "numRecords"],
        "PyDbLayerTableRecord": ["newRecord", "setName", "setColor", "setIsOff", "setIsLocked", "setIsFrozen", "isInUse"],
        "PyDbBlockTableRecord": ["appendAcDbEntity", "appendEntity", "entityIds", "iterEntities", "numEntities", "getEntityById", "__len__", "__iter__"],
        "PyDbPolyline": ["addVertexAt", "setPointAt", "getBulgeAt", "setBulgeAt", "setWidthsAt", "numVerts", "setClosed"],
        "PyDbBlockReference": ["setBlockTableRecord", "setPosition", "setRotation", "setScaleFactors"],
        "PyDbObject": ["erase", "close", "upgradeOpen", "downgradeOpen", "objectId", "handle", "isErased"],
        "PyDbEntity": ["setColorIndex", "setLayer", "setLinetype", "transformBy", "getGeomExtents"],
        "PyDbDatabase": ["objectIds", "saveAs", "save", "getBlockTable", "getLayerTable", "currentSpaceId", "approximateNumObjects"],
        "PyDbTrans": ["openObject", "addEntity", "commit", "abort", "currentSpace", "__enter__", "__exit__"],
        "PyDbAlignedDimension": ["setXLine1Point", "setXLine2Point", "setDimLinePoint", "setDimensionText"],
        "PyDbHatch": ["setPattern", "appendLoop", "setPatternScale", "setHatchObjectType", "evaluateHatch", "associative"],
        "PyDbMText": ["setLocation", "setContents", "setWidth", "setHeight", "setAttachment"],
        "PyDbText": ["setPosition", "setTextString", "setHeight", "setRotation", "setJustification"],
        "PyDbEllipse": ["setCenter", "setMajorRadius", "setMinorRadius", "setRadiusRatio", "setStartAngle", "setEndAngle"],
    },
    "Ed": {"": ["ssget", "sslength", "ssname", "sssetfirst", "ssgetfirst", "entget", "entmod", "entdel", "handent", "entlast", "acedSSGet", "command", "pyedSelect"]},
    "Ap": {"Document": ["saveAs", "save", "name", "database", "isNamedDrawing"], "DocumentManager": ["curDocument", "count", "documents"]},
}


def d(o):
    return ((getattr(o, "__doc__", "") or "").strip())[:700]


R = {}
for mod, classes in Q.items():
    m = getattr(P, mod)
    if "" in classes:
        R[mod] = {n: d(getattr(m, n, None)) for n in classes[""]}
        continue
    for cname, names in classes.items():
        c = getattr(m, cname, None)
        R.setdefault(mod, {})[cname] = {n: d(getattr(c, n, None)) for n in names}

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(R, f, ensure_ascii=False, indent=1, default=str)
print("[PyArx] dump_methods done")
