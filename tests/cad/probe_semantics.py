# -*- coding: utf-8 -*-
"""语义探针：逐条确认 PyArxCore 真实调用形态（每条独立 try，绝不互相污染）。"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_probe.json"
R = {"probes": [], "done": False}
db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d


def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def probe(name, fn):
    rec = {"name": name, "ok": False, "err": None, "data": None}
    try:
        rec["data"] = fn()
        rec["ok"] = True
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
    R["probes"].append(rec)
    flush()


def make_line():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        return ln.objectId()


LINE_ID = make_line()
flush()

R["line_id"] = LINE_ID
flush()


def p1():
    o = db.openObject(LINE_ID, 0)
    try:
        cls = o.className()
        c2 = P.Db.PyDbLine.cast(o)
        return {"type": type(o).__name__, "className": str(cls),
                "cast_type": type(c2).__name__, "cast_ok": not c2.isNull(),
                "end": [c2.endPoint().x, c2.endPoint().y]}
    finally:
        o.close()


probe("openObject_cast", p1)


def p2():
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(LINE_ID, 0)
        try:
            return {"type": type(o).__name__, "className": str(o.className())}
        finally:
            o.close()


probe("trans_openObject", p2)


def p3():
    lt = db.getLayerTable(0)
    try:
        r = lt.getAt("0")
        try:
            return {"type": type(r).__name__, "name": str(r.getName())}
        finally:
            r.close()
    finally:
        lt.close()


probe("layerTable_getAt", p3)


def p4():
    bt = db.getBlockTable(0)
    try:
        r = bt.getAt("*Model_Space", 0)
        try:
            return {"type": type(r).__name__, "len": len(r)}
        finally:
            r.close()
    finally:
        bt.close()


probe("blockTable_getAt", p4)


def p5():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    try:
        ids = btr.entityIds()
        out = {"n": len(ids), "numEntities": btr.numEntities(), "len": len(btr)}
        vals = btr.values(1)
        out["values_type"] = type(vals).__name__ if not isinstance(vals, list) else "list"
        out["values_n"] = len(vals)
        if vals:
            out["value0"] = type(vals[0]).__name__
        return out
    finally:
        btr.close()


probe("btr_values", p5)


def p6():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    try:
        ids = btr.entityIds()
        if not ids:
            return "empty"
        e = btr[ids[0]]
        out = {"getitem_type": type(e).__name__, "null": e.isNull()}
        e.close()
        return out
    finally:
        btr.close()


probe("btr_getitem", p6)


def p7():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    try:
        it = iter(btr)
        e = next(it)
        out = {"iter_type": type(e).__name__, "class": str(e.className())}
        e.setColorIndex(4)
        out["color_after"] = e.colorIndex()
        return out
    finally:
        btr.close()


probe("btr_iter_one", p7)


def p8():
    s = P.Db.PyDb3dSolid()
    r = s.createBox(10.0, 10.0, 10.0)
    m = P.Ge.AcGeMatrix3d()
    ok = m.setToTranslation(V3(400, 0, 0))
    r2 = s.transformBy(m) if ok else None
    with P.Db.PyDbTrans() as tr:
        add = tr.addEntity(s)
        oid = s.objectId()
    return {"create": r, "transform": r2, "add": add, "id": oid}


probe("solid3d_box", p8)


def p9():
    h = db.openObject(LINE_ID, 0)
    handle = str(h.handle())
    h.close()
    en = P.Ed.handent(handle)
    dxf = P.Ed.entget(en) if en is not None else None
    return {"handle": handle, "handent": en is not None,
            "entget_type": type(dxf).__name__ if dxf is not None else None,
            "codes": sorted([int(k) for k in dxf.keys()])[:12] if isinstance(dxf, dict) else None}


probe("ed_handent_entget", p9)


def p10():
    ss = P.Ed.ssget("X", None, None, [(0, "LINE")])
    return {"ss": None if ss is None else type(ss).__name__,
            "n": None if ss is None else P.Ed.sslength(ss)}


probe("ed_ssget_X", p10)


def p11():
    ok1 = P.Ed.sssetfirst([LINE_ID])
    g = P.Ed.ssgetfirst()
    ok2 = P.Ed.sssetfirst(None)
    return {"set": ok1, "get_type": type(g).__name__, "get_len": len(g),
            "get_first_len": None if g is None else len(g[1]), "clear": ok2}


probe("ed_sssetfirst", p11)


def p12():
    lt = db.getLayerTable(1)
    try:
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName("PYX_PROBE_LAYER")
        ok = lt.add(rec)
        oid = rec.objectId()
        has = lt.has("PYX_PROBE_LAYER")
        return {"add": ok, "id": oid, "has": has}
    finally:
        lt.close()


probe("layer_create", p12)

R["done"] = True
flush()
print("[PyArx] probe done")
