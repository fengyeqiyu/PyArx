# -*- coding: utf-8 -*-
"""pyarx_api_test_batch3.py — 第三批：openObject / 事务 / Ed None 校验路径（无实体变更链）。"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch3.json"
R = {}

db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d


def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def run(k, fn):
    rec = {"created": False, "handle": None, "readback": None,
           "blocked": False, "ok": False, "err": None}
    try:
        out = fn()
        if isinstance(out, dict):
            rec.update(out)
        else:
            rec["readback"] = out
        rec["ok"] = rec["created"] and (rec["readback"] is not None)
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:200]
    R[k] = rec
    flush()


def line_count():
    return len(db.objectIds("AcDbLine"))


def v_open_null_id():
    o = db.openObject(0, 0)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_open_bad_id():
    o = db.openObject(999999999, 0)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_open_bad_mode():
    ids = db.objectIds("AcDbLine")
    if len(ids) == 0:
        return {"created": False, "blocked": True, "readback": {"no_entity": True}}
    o = db.openObject(ids[-1], 9)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_trans_open_null():
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(0, 0)
    return {"created": False, "blocked": True, "readback": {"isNone": o is None}}


def v_trans_open_bad_mode():
    ids = db.objectIds("AcDbLine")
    if len(ids) == 0:
        return {"created": False, "blocked": True, "readback": {"no_entity": True}}
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(ids[-1], 9)
    return {"created": False, "blocked": True, "readback": {"isNone": o is None}}


def v_trans_exception_rollback():
    before = line_count()
    try:
        with P.Db.PyDbTrans() as tr:
            ln = P.Db.PyDbLine(P3(0, -100, 0), P3(10, -100, 0))
            tr.addEntity(ln)
            raise RuntimeError("simulate script error")
    except RuntimeError:
        pass
    after = line_count()
    return {"created": False, "blocked": True,
            "readback": {"before": before, "after": after, "rolled_back": after == before}}


def v_trans_normal_commit():
    before = line_count()
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, -200, 0), P3(10, -200, 0))
        tr.addEntity(ln)
    after = line_count()
    return {"created": True, "readback": {"before": before, "after": after,
                                          "committed": after == before + 1}}


def v_ed_none():
    return {"created": False, "blocked": True,
            "readback": {"entget": P.Ed.entget(None),
                         "entdel": P.Ed.entdel(None),
                         "entupd": P.Ed.entupd(None),
                         "handent": P.Ed.handent(None)}}


for k, fn in list(globals().items()):
    if k.startswith("v_"):
        run(k, fn)

R["done"] = True
flush()
print("[PyArx] batch3 done -> " + OUT)
