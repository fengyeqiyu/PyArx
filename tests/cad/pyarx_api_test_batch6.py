# -*- coding: utf-8 -*-
"""batch6：迭代器/字典协议 + 默认写模式串联 + entmod/entdel 异常路径。

每步独立落盘 JSON，卡住时最后一个 key 即卡点。
"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch6.json"
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


def step(k, fn):
    rec = {"step": k, "created": False, "readback": None,
           "blocked": False, "ok": False, "err": None}
    try:
        out = fn()
        if isinstance(out, dict):
            rec.update(out)
        else:
            rec["readback"] = out
        rec["ok"] = rec["created"] and rec["readback"] is not None
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:220]
    R["steps"].append(rec)
    flush()


BTR = {"id": 0}


def btr_setup():
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()  # 默认 kForWrite
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T6")
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 4.0)
        btr.appendAcDbEntity(c)
        bt.add(btr)
        BTR["id"] = btr.objectId()
    return {"created": True, "readback": {"btr_id": BTR["id"]}}


def btr_protocol():
    btr = P.Db.PyDbBlockTableRecord(BTR["id"], 1)
    n = len(btr)
    ids = btr.entityIds()
    vals = btr.values(1)
    items = btr.items(1)
    keys = btr.keys()
    eid = ids[0]
    by_key = btr[eid]
    by_get = btr.get(eid, None)
    missing = btr.get(999999999, None)
    contains = eid in btr
    # 串联：遍历块内实体直接改颜色 3，不传 mode
    for ent in btr:
        ent.setColorIndex(3)
    first = btr[eid]
    return {"created": True,
            "readback": {"len": n, "ids": ids, "values": len(vals),
                          "items": len(items), "keys": len(keys),
                          "by_key_ok": not by_key.isNull(),
                          "get_ok": not by_get.isNull(),
                          "missing_none": missing is None,
                          "contains": contains,
                          "color_after": first.colorIndex()}}


def layer_protocol():
    lt = db.getLayerTable()  # 默认 kForWrite
    names = lt.keys()
    vals = lt.values(1)
    items = lt.items(1)
    zero = lt["0"]
    contains = "0" in lt
    missing = lt.get("PYX_NO_SUCH_LAYER_XYZ", None)
    return {"created": True,
            "readback": {"len": len(lt), "keys": len(names),
                          "values": len(vals), "items": len(items),
                          "zero_ok": not zero.isNull(),
                          "contains": contains, "missing_none": missing is None}}


ED = {"ename": None, "handle": None}


def ed_create():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        ED["handle"] = str(ln.handle())
    return {"created": True, "handle": ED["handle"]}


def ed_handent():
    ED["ename"] = P.Ed.handent(ED["handle"])
    return {"created": True, "readback": {"is_none": ED["ename"] is None}}


def ed_entmod():
    dxf = P.Ed.entget(ED["ename"])
    full = dict(dxf)
    full[62] = 1
    ok = P.Ed.entmod(full)
    after = P.Ed.entget(ED["ename"])
    return {"created": True, "readback": {"entmod_ok": ok,
                                          "color": after.get(62) if after else None}}


def ed_entdel_path():
    del_ok = P.Ed.entdel(ED["ename"])
    after_get = P.Ed.entget(ED["ename"])
    dxf2 = after_get if after_get else {0: "LINE", 62: 1}
    mod_del = P.Ed.entmod(dxf2)
    restore = P.Ed.entdel(ED["ename"])
    return {"created": True,
            "readback": {"del_ok": del_ok,
                         "entget_after_erase": None if after_get is None else "non-null",
                         "entmod_on_erased": mod_del, "restore_ok": restore}}


step("btr_setup", btr_setup)
step("btr_protocol", btr_protocol)
step("layer_protocol", layer_protocol)
step("ed_create", ed_create)
step("ed_handent", ed_handent)
step("ed_entmod", ed_entmod)
step("ed_entdel_path", ed_entdel_path)

R["done"] = True
flush()
print("[PyArx] batch6 done -> " + OUT)
