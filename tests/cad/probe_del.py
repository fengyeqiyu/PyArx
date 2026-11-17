# -*- coding: utf-8 -*-
"""删除专项：同一包装上 erase(true/false)，并用计数确认。"""
import datetime, json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\probe_del.json"
OUT = {"steps": [], "done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)


def step(tag, fn):
    rec = {"tag": tag}
    try:
        v = fn()
        rec["ok"] = True
        rec["v"] = v if isinstance(v, (int, float, bool, str, type(None))) else str(v)[:80]
        log("OK", "%s -> %s" % (tag, rec["v"]))
    except Exception as e:
        rec["ok"] = False
        rec["v"] = type(e).__name__ + ": " + str(e)[:120]
        log("FAIL", "%s -> %s" % (tag, rec["v"]))
    OUT["steps"].append(rec)
    flush()


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
ids = DB.objectIds("AcDbPoint")
OUT["point_ids"] = len(ids)
if not ids:
    new_id = sp.appendAcDbEntityId(P.Db.PyDbPoint(P.Ge.AcGePoint3d(300.0, 0.0, 0.0)))
    OUT["created"] = new_id
    ids = [new_id]
    log("坑点", "上一轮已 erase 的点无法用 getEntityById 再打开（返回空包装），删除测试需先造新实体")
log("====", "probe_del 开始 points=%d entities=%d" % (len(ids), len(DB.objectIds("AcDbEntity"))))
if ids:
    e = sp.getEntityById(ids[0], 1)
    step("isNull", lambda: e.isNull())
    step("isErased.before", lambda: e.isErased())
    step("erase", lambda: e.erase())
    step("isErased.after", lambda: e.isErased())
    step("count.erased", lambda: len(DB.objectIds("AcDbEntity")))
    step("erase.undo", lambda: e.erase(False))
    step("isErased.restored", lambda: e.isErased())
    step("count.restored", lambda: len(DB.objectIds("AcDbEntity")))
    step("close", lambda: e.close())
step("sp.close", lambda: sp.close())
OUT["done"] = True
flush()
log("====", "probe_del 结束")
print("[PyArx] probe_del done")
