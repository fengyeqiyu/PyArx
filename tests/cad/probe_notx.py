# -*- coding: utf-8 -*-
"""探针：不用事务，直接对符号表 add 记录（看是否落库、是否可按名读回）。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_notx.json"
NAME = "PY_LAYER_NOTX"
LOG = []


def log(tag, v):
    LOG.append({"step": tag, "value": v})
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(LOG, f, ensure_ascii=False, indent=1, default=str)


def run():
    db = P.Db.curDb()
    lt = db.getLayerTable(1)
    log("has", str(lt.has(NAME)))
    if not lt.has(NAME):
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(NAME)
        rec.setColor(20, 120, 220)
        ok = lt.add(rec)
        log("add", str(ok))
        log("rec.id", str(rec.objectId()))
        rec.close()
    lt.close()

    lt2 = db.getLayerTable(0)
    r = lt2.getAt(NAME)
    log("getAt.isNull", str(r.isNull()))
    if not r.isNull():
        log("getAt.name", str(r.getName()))
        r.close()
    log("names_has", str(NAME in lt2.names()))
    lt2.close()
    log("done", True)


run()
