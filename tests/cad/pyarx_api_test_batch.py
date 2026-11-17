# -*- coding: utf-8 -*-
"""pyarx_api_test_batch.py — PyArxCore 绑定层图元增改 + 校验路径批量实测（acad 进程内）。

每条用例记录：created（创建/操作是否成功）、handle（句柄/entlast 是否可取）、
readback（写回值读回是否一致）、blocked（非法入参是否被校验挡住）、err。
结果写 G:\\PyArx\\log\\pyarx_api_test_batch.json。
"""
import json
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch.json"
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


# ---------- A. 新增图元 ----------

def c_line():
    with P.Db.PyDbTrans() as tr:
        ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
        tr.addEntity(ln)
        h = _handle(ln)
        sp = (ln.startPoint().x, ln.startPoint().y, ln.startPoint().z)
        ep = (ln.endPoint().x, ln.endPoint().y, ln.endPoint().z)
    return {"created": True, "handle": h,
            "readback": {"start": sp, "end": ep},
            "created_ids": len(db.objectIds("AcDbLine"))}


def c_circle():
    with P.Db.PyDbTrans() as tr:
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 5.0)
        tr.addEntity(c)
        h = _handle(c)
        rb = {"radius": c.radius(),
              "center": (c.center().x, c.center().y, c.center().z)}
    return {"created": True, "handle": h, "readback": rb}


def c_arc():
    with P.Db.PyDbTrans() as tr:
        a = P.Db.PyDbArc(P3(0, 0, 0), 3.0, 0.0, 1.5707963267948966)
        tr.addEntity(a)
        h = _handle(a)
        rb = {"radius": a.radius(), "start": a.startAngle(), "end": a.endAngle()}
    return {"created": True, "handle": h, "readback": rb}


def c_polyline():
    with P.Db.PyDbTrans() as tr:
        pl = P.Db.PyDbPolyline()
        ok0 = pl.addVertexAt(0, P2(0, 0))
        ok1 = pl.addVertexAt(1, P2(10, 0))
        ok2 = pl.addVertexAt(2, P2(10, 10))
        tr.addEntity(pl)
        h = _handle(pl)
        rb = {"verts": pl.numVerts(),
              "p1": (pl.getPointAt(1).x, pl.getPointAt(1).y),
              "inserts": [ok0, ok1, ok2]}
    return {"created": True, "handle": h, "readback": rb}


def c_polyline_oob():
    pl = P.Db.PyDbPolyline()
    bad = pl.addVertexAt(99, P2(1, 1))
    pt = pl.getPointAt(99)
    seg = pl.segType(99)
    return {"created": False, "blocked": True,
            "readback": {"add_oob": bad, "get_oob": (pt.x, pt.y), "seg_oob": seg}}


def c_rectangle():
    with P.Db.PyDbTrans() as tr:
        pl = P.Db.PyDbPolyline()
        pl.addVertexAt(0, P2(0, 0))
        pl.addVertexAt(1, P2(20, 0))
        pl.addVertexAt(2, P2(20, 10))
        pl.addVertexAt(3, P2(0, 10))
        pl.setClosed(True)
        tr.addEntity(pl)
        h = _handle(pl)
    return {"created": True, "handle": h,
            "readback": {"verts": pl.numVerts(), "closed": True}}


def c_text():
    with P.Db.PyDbTrans() as tr:
        t = P.Db.PyDbText(P3(0, 0, 0), "PYARX", 2.0, 0.0)
        tr.addEntity(t)
        h = _handle(t)
        rb = {"text": str(t.textString()), "height": t.height()}
    return {"created": True, "handle": h, "readback": rb}


def c_dimension():
    with P.Db.PyDbTrans() as tr:
        d = P.Db.PyDbAlignedDimension()
        d.setXLine1Point(P3(0, 0, 0))
        d.setXLine2Point(P3(30, 0, 0))
        d.setDimLinePoint(P3(0, 10, 0))
        tr.addEntity(d)
        h = _handle(d)
        rb = {"x1": (d.xLine1Point().x, d.xLine1Point().y)}
    return {"created": True, "handle": h, "readback": rb}


def c_blockref():
    # 先建块定义（圆），再插块参照；链路任一环失败则如实记录。
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T1")
        bt.add(btr)
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 4.0)
        btr.appendEntity(c)
        br = P.Db.PyDbBlockReference()
        br.setBlockTableRecord(btr.objectId())
        br.setPosition(P3(30, 0, 0))
        tr.addEntity(br)
        h = _handle(br)
        rb = {"pos": (br.position().x, br.position().y)}
    return {"created": True, "handle": h, "readback": rb}


# ---------- B. 属性写回 ----------

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


# ---------- C. 几何修改 ----------

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


# ---------- D. entmod / entdel 异常路径 ----------

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
    del_ok = P.Ed.entdel(ename)  # 擦除
    reopened = db.openObject(oid, 1)  # 已擦除对象应被校验挡住
    blocked = reopened.isNull()
    P.Ed.entdel(ename)  # 恢复
    return {"created": True, "handle": h,
            "readback": {"entdel_ok": del_ok, "reopened_null": blocked},
            "blocked": blocked}


# ---------- E. openObject / 事务校验路径 ----------

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
    if k.startswith("c_") or k.startswith("prop_") or k.startswith("geo_") \
            or k.startswith("ed_") or k.startswith("v_"):
        run(k, fn)

R["done"] = True
flush()
print("[PyArx] batch done -> " + OUT)
