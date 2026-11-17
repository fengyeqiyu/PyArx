# -*- coding: utf-8 -*-
"""batch4：补测非法 mode（先建一条直线取有效 ObjectId）。"""
import json, os, traceback
import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch4.json"
R = {}
db = P.Db.curDb(); P3 = P.Ge.AcGePoint3d

def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)

def run(k, fn):
    rec = {"created": False, "readback": None, "blocked": False, "ok": False, "err": None}
    try:
        out = fn()
        if isinstance(out, dict): rec.update(out)
        else: rec["readback"] = out
        rec["ok"] = rec["created"] and rec["readback"] is not None
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
    R[k] = rec; flush()

def setup():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
    return db.objectIds("AcDbLine")[-1]

def v_open_bad_mode():
    oid = setup()
    o = db.openObject(oid, 9)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}

def v_trans_open_bad_mode():
    oid = setup()
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(oid, 9)
    return {"created": False, "blocked": True, "readback": {"isNone": o is None}}

run("v_open_bad_mode", v_open_bad_mode)
run("v_trans_open_bad_mode", v_trans_open_bad_mode)
R["done"] = True; flush()
print("[PyArx] batch4 done")
