# -*- coding: utf-8 -*-
"""batch15 PyRx 风格 API：db.addToModelspace / addToBlock / openObject / sysVar。不保存。"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch15.json"
IDS = {}
R = {"items": [], "done": False}


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
log("====", "batch15 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_add():
    out = {}
    out["line"] = DB.addToModelspace(P.Db.PyDbLine(P3(0, 200), P3(100, 200)))
    out["circle"] = DB.addToModelspace(P.Db.PyDbCircle(P3(150, 250), V3(0, 0, 1), 15))
    out["text"] = DB.addToModelspace(P.Db.PyDbText(P3(0, 300), "PyRxStyle", 3.0, 0.0))
    out["point"] = DB.addToModelspace(P.Db.PyDbPoint(P3(300, 300)))
    out["cur"] = DB.addToCurrentspace(P.Db.PyDbCircle(P3(400, 250), V3(0, 0, 1), 5))
    out["block"] = DB.addToBlock(DB.currentSpaceId(), P.Db.PyDbLine(P3(0, 350), P3(80, 350)))
    out["count"] = len(DB.objectIds("AcDbEntity"))
    log("坑点", "db.addToModelspace/addToBlock 是 PyRx 风格的裸 appendAcDbEntity；返回 id 有效但对象归属需看持久化结果")
    return out


def t2_open():
    out = {}
    oid = IDS or None
    return out


def t3_read():
    out = {}
    for name in ("line", "circle", "text"):
        if name not in IDS:
            continue
    return out


def _read(kind):
    ids = DB.objectIds(kind)
    return ids


def t3_openObject():
    out = {"counts": {}}
    for cls in ("AcDbLine", "AcDbCircle", "AcDbText", "AcDbPoint"):
        out["counts"][cls] = len(DB.objectIds(cls))
    ids = DB.objectIds("AcDbCircle")
    for oid in ids:
        obj = DB.openObject(oid, 0)
        if obj.isNull():
            out.setdefault("null", []).append(oid)
            continue
        c = P.Db.PyDbCircle.cast(obj)
        out.setdefault("circles", []).append({"r": c.radius(), "center": [c.center().x, c.center().y]})
        obj.close()
    log("坑点", "PyRx 用 Db.Line(id, mode) 构造；本工程无 id 构造，等价写法是 db.openObject(id, mode) + PyDbXxx.cast")
    return out


def t4_sysvar():
    out = {}
    out["osmode"] = DB.getSysVar("OSMODE")
    out["set_test"] = DB.setSysVar("GRIDMODE", 1)
    out["gridmode"] = DB.getSysVar("GRIDMODE")
    out["restore"] = DB.setSysVar("GRIDMODE", 0)
    return out


item("P1_add", "db.addToModelspace/addToCurrentspace/addToBlock 入库", t1_add)
item("P2_read", "db.openObject + cast 读取圆几何", t3_openObject)
item("P3_sysvar", "getSysVar/setSysVar", t4_sysvar)

IDS.clear()
R["done"] = True
flush()
log("====", "batch15 结束")
print("[PyArx] batch15 done")
