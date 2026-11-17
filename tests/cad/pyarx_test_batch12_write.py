# -*- coding: utf-8 -*-
"""batch12 写：图元走无事务 appendAcDbEntityId；图层/块表走最小事务（否则不落库）。"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch12_write.json"
IDS = {}
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
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False, default=str)[:300])
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:220]
        log("FAIL", name + " " + rec["err"])
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()


def P2(x, y):
    return P.Ge.AcGePoint2d(float(x), float(y))


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def V3(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
log("====", "batch12_write 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_draw():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    makers = [
        ("line", lambda: P.Db.PyDbLine(P3(0, 0), P3(100, 0))),
        ("circle", lambda: P.Db.PyDbCircle(P3(50, 50), V3(0, 0, 1), 20)),
        ("arc", lambda: P.Db.PyDbArc(P3(0, 60), 30, 0.0, 3.14159)),
        ("text", lambda: P.Db.PyDbText(P3(0, -20), "PyArx T12", 4.0, 0.0)),
        ("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50), V3(0, 0, 1), V3(25, 0, 0), 0.4)),
        ("point", lambda: P.Db.PyDbPoint(P3(150, 0))),
        ("solid", lambda: P.Db.PyDbSolid(P3(0, 100), P3(20, 100), P3(20, 120), P3(0, 120))),
    ]
    for k, mk in makers:
        IDS[k] = sp.appendAcDbEntityId(mk())
        out[k] = IDS[k]
    out["num"] = sp.numEntities()
    sp.close()
    out["db"] = len(DB.objectIds("AcDbEntity"))
    return out


def t2_layer():
    out = {}
    with P.Db.PyDbTrans() as tr:
        lt = DB.getLayerTable(1)
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        out["setName"] = rec.setName("PYX_T12_LAYER")
        rec.setColor(200, 80, 40)
        out["add"] = lt.add(rec)
        out["id"] = rec.objectId()
        lt.close()
    out["has_after"] = DB.getLayerTable(0).has("PYX_T12_LAYER")
    log("坑点", "符号表记录必须放在 PyDbTrans 里创建，裸 add() 不落库；且 getSymbolTable 期间只开一次并 close")
    return out


def t3_block():
    out = {}
    with P.Db.PyDbTrans() as tr:
        bt = DB.getBlockTable(1)
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T12_BLOCK")
        c1 = P.Db.PyDbCircle(P3(0, 0), V3(0, 0, 1), 10)
        c2 = P.Db.PyDbLine(P3(-10, 0), P3(10, 0))
        out["child1"] = btr.appendAcDbEntity(c1)
        out["child2"] = btr.appendAcDbEntity(c2)
        IDS["blk"] = tr.addEntity(btr)
        if not IDS["blk"]:
            out["bt_add"] = bt.add(btr)
            IDS["blk"] = btr.objectId()
        out["blk"] = IDS["blk"]
        out["block_ents"] = btr.numEntities()
        bt.close()
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(IDS["blk"])
    br.setPosition(P3(500, 0))
    out["ref"] = sp.appendAcDbEntityId(br)
    out["space_num"] = sp.numEntities()
    sp.close()
    return out


item("T1_draw", t1_draw)
item("T2_layer", t2_layer)
item("T3_block", t3_block)

R["done"] = True
flush()
log("====", "batch12_write 结束")
print("[PyArx] batch12_write done")
