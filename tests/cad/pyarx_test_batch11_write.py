# -*- coding: utf-8 -*-
"""batch11 写：无事务 + appendAcDbEntityId（内部自动事务提交）。
跑完本脚本后另起一个进程跑 batch11_read 读回，才是真正的持久化验证。
"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch11_write.json"
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
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False)[:260])
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
log("====", "batch11_write 开始 space=%s entities=%s" % (SPACE, len(DB.objectIds("AcDbEntity"))))
flush()


def t1_draw():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    makers = [
        ("line", lambda: P.Db.PyDbLine(P3(0, 0), P3(100, 0))),
        ("circle", lambda: P.Db.PyDbCircle(P3(50, 50), V3(0, 0, 1), 20)),
        ("arc", lambda: P.Db.PyDbArc(P3(0, 60), 30, 0.0, 3.14159)),
        ("text", lambda: P.Db.PyDbText(P3(0, -20), "PyArx T11", 4.0, 0.0)),
        ("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50), V3(0, 0, 1), V3(25, 0, 0), 0.4)),
        ("point", lambda: P.Db.PyDbPoint(P3(150, 0))),
        ("solid", lambda: P.Db.PyDbSolid(P3(0, 100), P3(20, 100), P3(20, 120), P3(0, 120))),
    ]
    for k, mk in makers:
        e = mk()
        ret = sp.appendAcDbEntityId(e)
        IDS[k] = ret
        out[k] = ret
    out["num_in_record"] = sp.numEntities()
    out["close_record"] = sp.close()
    out["db_entities"] = len(DB.objectIds("AcDbEntity"))
    log("坑点", "PyDbBlockTableRecord.appendAcDbEntity 不注册新对象，实体在命令结束后丢失；必须用 appendAcDbEntityId / appendEntity")
    return out


def t2_layer():
    out = {}
    lt = DB.getLayerTable(1)
    rec = P.Db.PyDbLayerTableRecord.newRecord()
    out["setName"] = rec.setName("PYX_T11_LAYER")
    rec.setColor(200, 80, 40)
    out["add"] = lt.add(rec)
    out["layer_id"] = rec.objectId()
    out["close_lt"] = lt.close()
    log("坑点", "getAt 后想改记录属性必须先 upgradeOpen()（getAt 内部固定 kForRead 打开）")
    return out


def t3_block():
    out = {}
    bt = DB.getBlockTable(1)
    btr = P.Db.PyDbBlockTableRecord()
    btr.setName("PYX_T11_BLOCK")
    out["child1"] = btr.appendAcDbEntityId(P.Db.PyDbCircle(P3(0, 0), V3(0, 0, 1), 10))
    out["child2"] = btr.appendAcDbEntityId(P.Db.PyDbLine(P3(-10, 0), P3(10, 0)))
    out["block_id"] = btr.objectId()
    out["bt_add"] = bt.add(btr)
    out["blk"] = btr.objectId()
    out["close_bt"] = bt.close()
    if out["blk"] == 0:
        out["blk"] = bt.getIdAt("PYX_T11_BLOCK")
    IDS["blk"] = out["blk"]

    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    br = P.Db.PyDbBlockReference()
    br.setBlockTableRecord(IDS["blk"])
    br.setPosition(P3(500, 0))
    out["ref_id"] = sp.appendAcDbEntityId(br)
    out["space_num"] = sp.numEntities()
    out["close_sp"] = sp.close()
    return out


def t4_dbcount():
    return {"db_entities": len(DB.objectIds("AcDbEntity")),
            "filename": str(DB.getFilename())}


item("T1_draw", t1_draw)
item("T2_layer", t2_layer)
item("T3_block", t3_block)
item("T4_dbcount", t4_dbcount)

R["done"] = True
flush()
log("====", "batch11_write 结束")
print("[PyArx] batch11_write done")
