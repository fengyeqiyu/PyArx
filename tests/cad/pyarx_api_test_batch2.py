# -*- coding: utf-8 -*-
"""pyarx_api_test_batch2.py — 第二批：属性写回 / 几何修改 / entmod-entdel / 校验路径。"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch2.json"
R = {}

db = P.Db.curDb()
P2 = P.Ge.AcGePoint2d
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d


def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def run(k, fn):
    rec = {"created": False, "handle": None, "readback": None,
           "blocked": False, "ok": False, "err": None}
    try:
        out = fn()
        if isinstance(out, dict):
            rec.update(out)
        else:
            rec["readback"] = out
        rec["ok"] = rec["created"] and (rec["readback"] is not None)
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:300]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:200]
    R[k] = rec
    flush()


def line_count():
    return len(db.objectIds("AcDbLine"))


def _handle(ent):
    try:
        return str(ent.handle())
    except Exception:
        return None


def prop_color():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(5, 0, 0))
        tr.addEntity(ln)
        ln.setColorIndex(1)
        rb = {"colorIndex": ln.colorIndex()}
    return {"created": True, "readback": rb, "handle": _handle(ln)}


def prop_layer():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(5, 0, 0))
        tr.addEntity(ln)
        ln.setLayer("0")
        rb = {"layer": str(ln.layer())}
    return {"created": True, "readback": rb, "handle": _handle(ln)}


def prop_linetype():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(5, 0, 0))
        tr.addEntity(ln)
        ln.setLinetype("ByLayer")
        rb = {"linetype": str(ln.linetype())}
    return {"created": True, "readback": rb, "handle": _handle(ln)}


def geo_line():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        ln.setStartPoint(P3(20, 0, 0))
        rb = {"start": (ln.startPoint().x, ln.startPoint().y)}
    return {"created": True, "readback": rb, "handle": _handle(ln)}


def geo_circle():
    with P.Db.PyDbTrans() as tr:
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 5.0)
        tr.addEntity(c)
        c.setRadius(8.0)
        rb = {"radius": c.radius()}
    return {"created": True, "readback": rb, "handle": _handle(c)}


def geo_polyline():
    with P.Db.PyDbTrans() as tr:
        pl = P.Db.PyDbPolyline()
        pl.addVertexAt(0, P2(0, 0))
        pl.addVertexAt(1, P2(10, 0))
        tr.addEntity(pl)
        pl.setPointAt(1, P2(20, 20))
        rb = {"p1": (pl.getPointAt(1).x, pl.getPointAt(1).y)}
    return {"created": True, "readback": rb, "handle": _handle(pl)}


def ed_entget_entmod():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        h = str(ln.handle())
    ename = P.Ed.handent(h)
    dxf = P.Ed.entget(ename)
    ok_mod = P.Ed.entmod({8: "0", 62: 1})
    return {"created": True, "handle": h,
            "readback": {"entget_keys": sorted(dxf.keys()) if dxf else None,
                          "entmod_ok": ok_mod}}


def ed_entdel_then_modify():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        h = str(ln.handle())
        oid = ln.objectId()
    ename = P.Ed.handent(h)
    del_ok = P.Ed.entdel(ename)
    reopened = db.openObject(oid, 1)
    blocked = reopened.isNull()
    P.Ed.entdel(ename)
    return {"created": True, "handle": h,
            "readback": {"entdel_ok": del_ok, "reopened_null": blocked},
            "blocked": blocked}


def v_open_null_id():
    o = db.openObject(0, 0)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_open_bad_id():
    o = db.openObject(999999999, 0)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_open_bad_mode():
    ids = db.objectIds("AcDbLine")
    o = db.openObject(ids[-1], 9)
    return {"created": False, "blocked": True, "readback": {"isNull": o.isNull()}}


def v_trans_open_null():
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(0, 0)
    return {"created": False, "blocked": True, "readback": {"isNone": o is None}}


def v_trans_open_bad_mode():
    ids = db.objectIds("AcDbLine")
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(ids[-1], 9)
    return {"created": False, "blocked": True, "readback": {"isNone": o is None}}


def v_trans_exception_rollback():
    before = line_count()
    try:
        with P.Db.PyDbTrans() as tr:
            ln = P.Db.PyDbLine(P3(0, -100, 0), P3(10, -100, 0))
            tr.addEntity(ln)
            raise RuntimeError("simulate script error")
    except RuntimeError:
        pass
    after = line_count()
    return {"created": False, "blocked": True,
            "readback": {"before": before, "after": after, "rolled_back": after == before}}


def v_trans_normal_commit():
    before = line_count()
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, -200, 0), P3(10, -200, 0))
        tr.addEntity(ln)
    after = line_count()
    return {"created": True, "readback": {"before": before, "after": after,
                                          "committed": after == before + 1}}


def v_ed_none():
    return {"created": False, "blocked": True,
            "readback": {"entget": P.Ed.entget(None),
                         "entdel": P.Ed.entdel(None),
                         "entupd": P.Ed.entupd(None),
                         "handent": P.Ed.handent(None)}}


for k, fn in list(globals().items()):
    if k.startswith("prop_") or k.startswith("geo_") or k.startswith("ed_") \
            or k.startswith("v_"):
        run(k, fn)

R["done"] = True
flush()
print("[PyArx] batch2 done -> " + OUT)
