# -*- coding: utf-8 -*-
"""探针：classNameOf 判类型 + cast 类型不符时的表现。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\probe_type.json"
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
    cid = db.addToCurrentspace(P.Db.PyDbCircle(pt(0, 0), vec(0, 0, 1), 7))
    lid = db.addToCurrentspace(P.Db.PyDbLine(pt(0, 0), pt(10, 0)))
    log("cid", str(cid))
    log("lid", str(lid))
    log("type.cid", db.classNameOf(cid))
    log("type.lid", db.classNameOf(lid))

    obj = db.openObject(lid, 0)
    log("open.isNull", str(obj.isNull()))
    try:
        wrong = P.Db.PyDbCircle.cast(obj)
        log("cast.line_as_circle.isNull", str(wrong.isNull()))
        if not wrong.isNull():
            log("cast.radius", str(wrong.radius()))
            wrong.close()
    except Exception as e:
        log("cast.raised", type(e).__name__ + ": " + str(e)[:120])
    obj.close()

    obj2 = db.openObject(cid, 0)
    good = P.Db.PyDbCircle.cast(obj2)
    log("cast.circle.isNull", str(good.isNull()))
    log("cast.circle.radius", str(good.radius()))
    log("entities", len(db.objectIds("AcDbEntity")))
    log("done", True)


run()
