# -*- coding: utf-8 -*-
"""二分定位 saveAs 崩溃：逐级加料，每步独立数据库，看哪一步把进程打死。"""

import json
import os

import PyArxCore as P

TEMPLATE = r"C:\temp\pyarx_db_save_test.dwg"
OUT = r"C:\temp\save_bisect.json"
log = []


def newdb():
    db = P.Db.PyDbDatabase()
    db.readDwgFile(TEMPLATE)
    return db


def mklayer(db, name):
    lt = db.getLayerTable()
    if not lt.has(name):
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(name)
        rec.setColor(0, 160, 255)
        lt.add(rec)
    return lt


def size(p):
    return os.path.getsize(p) if os.path.exists(p) else 0


def step(name, fn):
    print("STEP " + name, flush=True)
    try:
        info = fn()
        log.append({"step": name, "ok": True, "info": info})
        print("  OK " + json.dumps(info, ensure_ascii=False), flush=True)
    except Exception as e:
        log.append({"step": name, "ok": False, "err": repr(e)})
        print("  ERR " + repr(e), flush=True)


def s1():
    p = r"C:\temp\bisect_1_doclib.dwg"
    st = str(P.Db.curDb().saveAs(p))
    return [st, size(p)]


def s2():
    db = newdb()
    p = r"C:\temp\bisect_2_newdb.dwg"
    st = str(db.saveAs(p))
    return [st, size(p)]


def s3():
    db = newdb()
    lt = mklayer(db, "BISECT_L")
    p = r"C:\temp\bisect_3_layer.dwg"
    st = str(db.saveAs(p))
    return [st, size(p), lt.has("BISECT_L")]


def s4():
    db = newdb()
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(50, 0, 0))
        tr.addEntity(ln)
    p = r"C:\temp\bisect_4_line.dwg"
    st = str(db.saveAs(p))
    return [st, size(p)]


def s5():
    db = newdb()
    lt = mklayer(db, "BISECT_L")
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(50, 0, 0))
        tr.addEntity(ln)
        ln.setLayerId(lt.getAt("BISECT_L").objectId())
    p = r"C:\temp\bisect_5_line_layer.dwg"
    st = str(db.saveAs(p))
    return [st, size(p)]


for name, fn in (("1_doclib_empty", s1), ("2_newdb_empty", s2),
                 ("3_newdb_layer", s3), ("4_newdb_line", s4),
                 ("5_newdb_line_layer", s5)):
    step(name, fn)

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print("DONE", flush=True)
