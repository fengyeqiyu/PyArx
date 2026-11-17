# -*- coding: utf-8 -*-
"""probe_del2：删除路径逐调用（新建点 → erase → 恢复），定位 unknown exception。"""
import json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\probe_del2.json"
OUT = {"steps": [], "done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % ("del2", tag, msg))


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


db = P.Db.curDb()
space = db.currentSpaceId()
holder = {}

step("add_point", lambda: holder.setdefault("id", db.addToCurrentspace(P.Db.PyDbPoint(P.Ge.AcGePoint3d(200.0, 520.0, 0.0)))))
step("ids_point", lambda: len(db.objectIds("AcDbPoint")))
step("open_btr", lambda: holder.setdefault("btr", P.Db.PyDbBlockTableRecord(space, 1)).__class__.__name__)
step("get_write", lambda: holder.setdefault("e", holder["btr"].getEntityById(holder["id"], 1)).__class__.__name__)
step("isNull", lambda: holder["e"].isNull())
step("isErased", lambda: holder["e"].isErased())
step("erase", lambda: holder["e"].erase())
step("isErased2", lambda: holder["e"].isErased())
step("count_erased", lambda: len(db.objectIds("AcDbEntity")))
step("points_erased", lambda: len(db.objectIds("AcDbPoint")))
step("erase_false", lambda: holder["e"].erase(False))
step("isErased3", lambda: holder["e"].isErased())
step("points_restored", lambda: len(db.objectIds("AcDbPoint")))
step("close_e", lambda: holder["e"].close())
step("close_btr", lambda: holder["btr"].close())

OUT["done"] = True
flush()
print("[PyArx] probe_del2 done")
