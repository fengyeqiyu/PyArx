# -*- coding: utf-8 -*-
"""batch11 读：新进程读回 batch11_write 写入的内容，验证持久化（不保存、不关图）。"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch11_read.json"
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
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False)[:300])
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:220]
        log("FAIL", name + " " + rec["err"])
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
log("====", "batch11_read 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_space():
    sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
    ids = sp.entityIds()
    out = {"numEntities": sp.numEntities(), "len": len(sp), "ids": len(ids)}
    cls = []
    for oid in ids:
        e = sp.getEntityById(oid, 0)
        cls.append(str(e.className()))
        e.close()
    out["classes"] = cls
    out["close"] = sp.close()
    return out


def t2_geom():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
    ids = sp.entityIds()
    for oid in ids:
        e = sp.getEntityById(oid, 0)
        name = str(e.className())
        rec = {"wrapper": type(e).__name__}
        ext = e.getGeomExtents() if hasattr(e, "getGeomExtents") else None
        if ext is not None:
            rec["ext"] = [str(ext[0]), str(ext[1])]
        if name == "AcDbCircle":
            c = P.Db.PyDbCircle.cast(e)
            rec["radius"] = c.radius()
            c.close()
        elif name == "AcDbLine":
            l = P.Db.PyDbLine.cast(e)
            rec["start"] = [l.startPoint().x, l.startPoint().y]
            rec["end"] = [l.endPoint().x, l.endPoint().y]
            l.close()
        elif name == "AcDbText":
            t = P.Db.PyDbText.cast(e)
            rec["text"] = str(t.textString())
            t.close()
        out.setdefault(name, []).append(rec)
    sp.close()
    log("坑点", "getEntityById 返回基类 PyDbEntity；取具体几何要 PyDbXxx.cast(ent)，且 cast 包装与原包装同一对象，只能 close 一次")
    return out


def t3_layer():
    out = {}
    lt = DB.getLayerTable(0)
    out["keys"] = len(lt.keys())
    out["has"] = lt.has("PYX_T11_LAYER")
    rec = lt.getAt("PYX_T11_LAYER")
    out["name"] = str(rec.getName())
    out["color"] = str(rec.color())
    out["upgrade"] = rec.upgradeOpen()
    out["isOff"] = rec.isOff()
    rec.close()
    out["close_lt"] = lt.close()
    return out


def t4_block():
    out = {}
    bt = DB.getBlockTable(0)
    out["block_keys"] = list(bt.keys())[:8]
    rec = bt.getAt("PYX_T11_BLOCK", 0)
    out["block_name"] = str(rec.getName())
    out["block_ents"] = len(rec.entityIds())
    rec.close()
    out["close_bt"] = bt.close()
    return out


def t5_modify():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    ids = sp.entityIds()
    for oid in ids:
        e = sp.getEntityById(oid, 1)
        if str(e.className()) == "AcDbCircle":
            e.setColorIndex(5)
            e.setLayer("PYX_T11_LAYER")
            out["circle_color"] = 5
            out["circle_layer"] = "PYX_T11_LAYER"
            e.close()
            break
        e.close()
    sp.close()
    return out


item("T1_space", t1_space)
item("T2_geom", t2_geom)
item("T3_layer", t3_layer)
item("T4_block", t4_block)
item("T5_modify", t5_modify)

R["done"] = True
flush()
log("====", "batch11_read 结束")
print("[PyArx] batch11_read done")
