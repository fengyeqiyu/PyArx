# -*- coding: utf-8 -*-
"""batch12 块表专项：块定义 + 块参照插入（最小事务，不保存）。"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch12_block.json"
R = {"items": [], "done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


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


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def V3(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
IDS = {}
log("====", "batch12_block 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_define():
    out = {}
    with P.Db.PyDbTrans() as tr:
        bt = DB.getBlockTable(1)
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T12_BLOCK")
        out["child1"] = btr.appendAcDbEntity(P.Db.PyDbCircle(P3(0, 0), V3(0, 0, 1), 10))
        out["child2"] = btr.appendAcDbEntity(P.Db.PyDbLine(P3(-10, 0), P3(10, 0)))
        out["bt_add"] = bt.add(btr)
        IDS["blk"] = btr.objectId()
        out["blk"] = IDS["blk"]
        out["block_ents"] = btr.numEntities()
        bt.close()
    return out


def t2_insert():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    br = P.Db.PyDbBlockReference()
    out["setBtr"] = br.setBlockTableRecord(IDS["blk"])
    out["setPos"] = br.setPosition(P3(500, 0))
    out["ref"] = sp.appendAcDbEntityId(br)
    out["space_num"] = sp.numEntities()
    sp.close()
    return out


def t3_readback():
    out = {}
    bt = DB.getBlockTable(0)
    out["keys"] = list(bt.keys())
    rec = bt.getAt("PYX_T12_BLOCK", 0)
    out["isNull"] = rec.isNull()
    if not rec.isNull():
        out["name"] = str(rec.getName())
        out["ents"] = len(rec.entityIds())
        rec.close()
    bt.close()
    return out


item("B1_define", t1_define)
item("B2_insert", t2_insert)
item("B3_readback", t3_readback)

R["done"] = True
flush()
log("====", "batch12_block 结束")
print("[PyArx] batch12_block done")
