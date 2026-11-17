# -*- coding: utf-8 -*-
"""batch17 修复验证：addToCurrentspace/addToModelspace/addToBlock 入库后是否留存。不保存。"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch17.json"
R = {"items": [], "done": False}
IDS = {}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def item(name, desc, fn):
    rec = {"item": name, "desc": desc, "ok": False}
    log("START", "%s | %s" % (name, desc))
    try:
        rec["data"] = fn()
        rec["ok"] = True
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False, default=str)[:320])
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:220]
        log("FAIL", name + " " + rec["err"])
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def V3(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


DB = P.Db.curDb()
log("====", "batch17 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_cur():
    IDS["c1"] = DB.addToCurrentspace(P.Db.PyDbCircle(P3(10, 500), V3(0, 0, 1), 8))
    IDS["c2"] = DB.addToCurrentspace(P.Db.PyDbCircle(P3(30, 500), V3(0, 0, 1), 5))
    return {"c1": IDS["c1"], "c2": IDS["c2"], "count": len(DB.objectIds("AcDbEntity"))}


def t2_model():
    IDS["l1"] = DB.addToModelspace(P.Db.PyDbLine(P3(0, 520), P3(50, 520)))
    return {"l1": IDS["l1"], "count": len(DB.objectIds("AcDbEntity"))}


def t3_block():
    IDS["p1"] = DB.addToBlock(DB.currentSpaceId(), P.Db.PyDbPoint(P3(70, 520)))
    return {"p1": IDS["p1"], "count": len(DB.objectIds("AcDbEntity"))}


def t4_open_now():
    out = {"open": [], "closed": []}
    for k, oid in IDS.items():
        obj = DB.openObject(oid, 0)
        if obj.isNull():
            out["closed"].append(k)
        else:
            out["open"].append(k)
            obj.close()
    return out


item("S1_currentspace", "addToCurrentspace ×2", t1_cur)
item("S2_modelspace", "addToModelspace ×1", t2_model)
item("S3_block", "addToBlock ×1", t3_block)
item("S4_open_now", "同命令内 openObject 检查", t4_open_now)

R["done"] = True
flush()
log("====", "batch17 结束")
print("[PyArx] batch17 done")
