# -*- coding: utf-8 -*-
"""探针 2：块定义用 db.addToBlock 追加子实体（不走事务内 append）。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_blockdef2.json"
NAME = "PYX_BLK_PROBE2"
LOG = []


def log(tag, v):
    LOG.append({"step": tag, "value": v})
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(LOG, f, ensure_ascii=False, indent=1, default=str)


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def vec(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def run():
    db = P.Db.curDb()
    bt = db.getBlockTable(1)
    log("has", str(bt.has(NAME)))
    blk = 0
    if not bt.has(NAME):
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName(NAME)
        ok = bt.add(btr)
        log("add", str(ok))
        blk = btr.objectId()
        log("blk", str(blk))
    bt.close()

    if blk:
        cir = P.Db.PyDbCircle(pt(0, 0), vec(0, 0, 1), 5)
        log("addToBlock.circle", str(db.addToBlock(blk, cir)))
        lig = P.Db.PyDbLine(pt(-5, 0), pt(5, 0))
        log("addToBlock.line", str(db.addToBlock(blk, lig)))

    bt2 = db.getBlockTable(0)
    rec = bt2.getAt(NAME, 0)
    log("isNull", str(rec.isNull()))
    if not rec.isNull():
        log("ents", len(rec.entityIds()))
        rec.close()
    bt2.close()
    log("done", True)


run()
