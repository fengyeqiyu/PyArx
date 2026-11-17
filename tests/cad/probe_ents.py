# -*- coding: utf-8 -*-
"""实体读取逐调用隔离：定位 getEntityById + cast 的失败点。"""
import datetime, json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\probe_ents.json"
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
        rec["v"] = v if isinstance(v, (int, float, bool, str, type(None), list)) else str(v)[:80]
        log("OK", "%s -> %s" % (tag, rec["v"]))
    except Exception as e:
        rec["ok"] = False
        rec["v"] = type(e).__name__ + ": " + str(e)[:120]
        log("FAIL", "%s -> %s" % (tag, rec["v"]))
    OUT["steps"].append(rec)
    flush()


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
log("====", "probe_ents 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))

sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
step("sp.open", lambda: type(sp).__name__)
ids = []
step("sp.num", lambda: sp.numEntities())
step("sp.entityIds", lambda: len(sp.entityIds()))
ids = sp.entityIds()

holders = {}
for i, oid in enumerate(ids[:3]):
    step("open#%d" % i, lambda oid=oid: holders.setdefault(i, {"e": sp.getEntityById(oid, 0)})["e"].__class__.__name__)
    step("isNull#%d" % i, lambda i=i: holders[i]["e"].isNull())
    step("base.className#%d" % i, lambda i=i: str(holders[i]["e"].className()))
    for cls_name in ("PyDbLine", "PyDbCircle", "PyDbArc", "PyDbText", "PyDbEllipse",
                     "PyDbPolyline", "PyDbPoint", "PyDbSolid"):
        cls = getattr(P.Db, cls_name)
        step("cast%s#%d" % (cls_name.replace("PyDb", ""), i),
             lambda cls=cls, i=i: holders.setdefault("w%d_%s" % (i, cls.__name__), {"w": cls.cast(holders[i]["e"])})["w"].__class__.__name__)
    step("ext#%d" % i, lambda i=i: [str(x) for x in holders[i]["e"].getGeomExtents()] if hasattr(holders[i]["e"], "getGeomExtents") else None)

step("sp.close", lambda: sp.close())
OUT["done"] = True
flush()
log("====", "probe_ents 结束")
print("[PyArx] probe_ents done")
