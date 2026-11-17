# -*- coding: utf-8 -*-
"""验收图灌数 headless 脚本：3 个实体在层 0、3 个在 LIVE_TEST，另存到 ctx_test.dwg。

PYRUNFILE 不注入 P，这里自己 import；只在副本上建图元，不碰原模板与 tests 原图。
"""
import json
import os

import PyArxCore as P

OUT = r"C:\temp\ctx_populate_out.json"
TARGET = os.environ.get("PYARX_CTX_TEST_DWG", r"C:\temp\pyarx_ctx_test.dwg")


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


result = {"target": TARGET, "handles": [], "layers": {}, "warnings": []}

try:
    db = P.Db.PyDbDatabase()
    db.readDwgFile(os.environ.get("PYARX_CTX_SOURCE_DWG", r"C:\temp\pyarx_db_save_test.dwg"))
    lt = db.getLayerTable()

    if not lt.has("LIVE_TEST"):
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName("LIVE_TEST")
        rec.setColor(255, 0, 0)
        if not lt.add(rec):
            raise RuntimeError("LIVE_TEST 层 add 返回 False")

    plan = [
        ("line", {"p1": [0, 0], "p2": [10, 0]}, "0"),
        ("line", {"p1": [0, 10], "p2": [20, 10]}, "0"),
        ("circle", {"center": [5, 5], "radius": 3.0}, "0"),
        ("line", {"p1": [0, 20], "p2": [30, 20]}, "LIVE_TEST"),
        ("circle", {"center": [10, 30], "radius": 5.0}, "LIVE_TEST"),
        ("circle", {"center": [40, 10], "radius": 4.0}, "LIVE_TEST"),
    ]

    with P.Db.PyDbTrans() as tr:
        for kind, args, layer in plan:
            if kind == "line":
                ent = P.Db.PyDbLine(pt(*args["p1"]), pt(*args["p2"]))
            else:
                ent = P.Db.PyDbCircle(pt(*args["center"]),
                                       P.Ge.AcGeVector3d(0.0, 0.0, 1.0),
                                       float(args["radius"]))
            baked = tr.addEntity(ent)
            if not baked:
                raise RuntimeError("addEntity 返回 False: %s %s" % (kind, args))
            if layer != "0":
                ok = False
                try:
                    ok = bool(ent.setLayerId(lt.getAt(layer).objectId()))
                except Exception as ex:
                    result["warnings"].append("setLayerId %s: %r" % (layer, ex))
                if not ok:
                    result["warnings"].append("setLayer 回退: target=%s actual=%s"
                                              % (layer, ent.layer()))
            result["handles"].append({"kind": kind, "layer": layer,
                                      "handle": str(ent.handle())})

    result["layers"]["0"] = sum(1 for h in result["handles"] if h["layer"] == "0")
    result["layers"]["LIVE_TEST"] = sum(1 for h in result["handles"] if h["layer"] == "LIVE_TEST")
    result["total"] = len(result["handles"])

    st = str(db.saveAs(TARGET))
    result["saveAs"] = st
    result["exists"] = os.path.exists(TARGET)
    result["size"] = os.path.getsize(TARGET) if os.path.exists(TARGET) else 0
except Exception:
    import traceback
    result["ok"] = False
    result["error"] = traceback.format_exc().splitlines()[-1]
    result["traceback"] = traceback.format_exc()[-1200:]

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, ensure_ascii=False, indent=1)
