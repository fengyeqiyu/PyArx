# -*- coding: utf-8 -*-
"""batch16 addToCurrentspace 专项：返回 id、立即计数、包装持有方式对留存的影响。不保存。"""
import datetime, json, gc, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch16.json"
R = {"items": [], "done": False}
HOLD = []


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
SPACE = DB.currentSpaceId()
log("====", "batch16 开始 entities=%s space=%s" % (len(DB.objectIds("AcDbEntity")), SPACE))
flush()


def a_temp():
    """包装即刻回收：addToCurrentspace 最小写法。"""
    oid = DB.addToCurrentspace(P.Db.PyDbCircle(P3(10, 400), V3(0, 0, 1), 8))
    return {"id": oid, "count": len(DB.objectIds("AcDbEntity"))}


def b_hold():
    """包装保持存活到脚本结束：看是否与 a 不同。"""
    ent = P.Db.PyDbLine(P3(20, 420), P3(60, 420))
    oid = DB.addToCurrentspace(ent)
    HOLD.append(ent)
    return {"id": oid, "count": len(DB.objectIds("AcDbEntity"))}


def c_baseline():
    """对照：btr.appendAcDbEntityId。"""
    btr = P.Db.PyDbBlockTableRecord(SPACE, 1)
    oid = btr.appendAcDbEntityId(P.Db.PyDbCircle(P3(40, 440), V3(0, 0, 1), 6))
    n = btr.numEntities()
    btr.close()
    return {"id": oid, "space_num": n, "count": len(DB.objectIds("AcDbEntity"))}


def d_gc():
    """强制 GC 后再计数，验证包装销毁是否连带删库。"""
    gc.collect()
    return {"count": len(DB.objectIds("AcDbEntity"))}


item("A_temp", "addToCurrentspace，包装即刻回收", a_temp)
item("B_hold", "addToCurrentspace，包装留全局", b_hold)
item("C_baseline", "对照 appendAcDbEntityId", c_baseline)
item("D_gc", "gc.collect 后计数", d_gc)

R["done"] = True
flush()
log("====", "batch16 结束")
print("[PyArx] batch16 done")
