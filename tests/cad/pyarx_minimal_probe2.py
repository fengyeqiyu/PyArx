# -*- coding: utf-8 -*-
"""最小实测：只覆盖 BTR 遍历、符号表 items、layerTable["0"]、contains。"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_minimal_probe2.json"
R = {"steps": []}
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(R, f, ensure_ascii=False, indent=1)

db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def step(name, fn):
    rec = {"step": name, "ok": False, "readback": None, "err": None}
    try:
        rec["readback"] = fn()
        rec["ok"] = True
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:220]
    R["steps"].append(rec)
    flush()


BTR_ID = {"id": 0}


def btr_protocol():
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T8")
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 4.0)
        btr.appendAcDbEntity(c)
        bt.add(btr)
        BTR_ID["id"] = btr.objectId()

    btr = P.Db.PyDbBlockTableRecord(BTR_ID["id"], 1)
    n = len(btr)
    for ent in btr:
        ent.setColorIndex(3)
    ids = btr.entityIds()
    first = btr[ids[0]]
    return {"len": n, "ids": ids, "color_after": first.colorIndex()}


def layer_protocol():
    lt = db.getLayerTable()
    keys = lt.keys()
    items = lt.items()
    del keys, items
    import gc
    gc.collect()
    zero = lt["0"]
    contains = "0" in lt
    return {"len": len(lt), "keys": len(keys), "items": len(items),
            "zero_ok": not zero.isNull(), "contains": contains,
            "missing_none": lt.get("PYX_NO_SUCH_LAYER_XYZ", None) is None}


step("btr_protocol", btr_protocol)
step("layer_protocol", layer_protocol)
R["done"] = True
flush()
print("[PyArx] minimal probe2 done -> " + OUT)
