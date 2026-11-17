# -*- coding: utf-8 -*-
"""最小验证：无事务 append 后 close，再读模式重开读回。全量日志 pyarx_learn.log"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch10.json"
R = {"items": [], "done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1)


def item(name, fn):
    rec = {"item": name, "ok": False}
    log("START", name)
    try:
        rec["data"] = fn()
        rec["ok"] = True
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False)[:200])
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:200]
        log("FAIL", name + " " + rec["err"])
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
IDS = {}
log("====", "batch10 开始 space=%s entities=%s" % (SPACE, len(DB.objectIds("AcDbEntity"))))
flush()


def t1_write():
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    out = {}
    for k, mk in (("line", lambda: P.Db.PyDbLine(P3(0, 0), P3(100, 0))),
                  ("circle", lambda: P.Db.PyDbCircle(P3(50, 50), P3(0, 0, 1), 20)),
                  ("text", lambda: P.Db.PyDbText(P3(0, -20), "PyArx T10", 4.0, 0.0))):
        e = mk()
        out[k + "_append"] = sp.appendAcDbEntity(e)
        IDS[k] = e.objectId()
        out[k + "_id"] = IDS[k]
        e.close()
        out[k + "_closed"] = True
    out["num_in_record"] = sp.numEntities()
    out["close_record"] = sp.close()
    return out


def t2_read():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
    out["numEntities"] = sp.numEntities()
    out["len"] = len(sp)
    out["ids_n"] = len(sp.entityIds())
    for k, oid in list(IDS.items()):
        e = sp.getEntityById(oid, 0)
        out[k] = {"null": e.isNull(), "cls": str(e.className()),
                  "wrapper": type(e).__name__}
        e.close()
    n = 0
    for e in sp.iterEntities(0, True):
        n += 1
    out["iter_n"] = n
    out["close_record"] = sp.close()
    return out


def t3_dbcount():
    return {"db_entities": len(DB.objectIds("AcDbEntity")),
            "db_all": DB.approximateNumObjects()}


item("T1_write", t1_write)
item("T2_read", t2_read)
item("T3_dbcount", t3_dbcount)

R["done"] = True
flush()
log("====", "batch10 结束")
print("[PyArx] batch10 done")
