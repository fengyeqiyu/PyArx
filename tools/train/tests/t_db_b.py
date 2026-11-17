# -*- coding: utf-8 -*-
"""Db 批次 B：BlockReference / Hatch / Table / Viewport 四类断言 + 失败面。"""
import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

ACCUM = r"G:\PyArx\tools\train\data\shard_db_accum.json"
L = "Db"


def pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def pt2(x, y):
    return P.Ge.AcGePoint2d(float(x), float(y))


def p3(p):
    if hasattr(p, "x"):
        return (p.x, p.y, p.z)
    return tuple(p)


def closed_loop(cls, mod, configure=None, label=""):
    db = P.Db.curDb()
    before = mlab.ent_count()
    e = cls()
    if configure:
        configure(e)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        h = e.handle()
        oid = e.objectId()
        in_tx = "handle=%s objectId=%s" % (h, oid)
    after = mlab.ent_count()
    assert str(h) not in ("0", ""), "句柄为 0，未真正入库"
    assert after > before, "commit 后实体数没增长: %d -> %d" % (before, after)
    return "%s before=%d after=%d %s" % (label, before, after, in_tx)


def readback(oid):
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(oid)
        return "openObject 读回 type=%s handle=%s" % (type(o).__name__, o.handle())


def exists_ctor(mod, cls, forms):
    assert hasattr(P.Db, mod), "P.Db 无 %s" % mod
    inst, form = mlab.ctor(cls, *forms)
    return "构造 %s type=%s.%s members=%d" % (
        form, type(inst).__module__, type(inst).__name__,
        len([x for x in dir(inst) if not x.startswith("_")]))


# ================================================================ PyDbBlockReference
M = "PyDbBlockReference"
C = P.Db.PyDbBlockReference

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def br_prop():
    e = C()
    e.setPosition(pt(7.0, 8.0, 0.0))
    e.setRotation(0.5)
    e.setScaleFactors(P.Ge.AcGeScale3d(1.5, 1.5, 1.5))
    p = p3(e.position())
    r = e.rotation()
    sf = e.scaleFactors()
    return "position=(%.3f,%.3f,%.3f) rotation=%.3f scaleFactors=%s(%s) blockTableRecordId=%s" % (
        p[0], p[1], p[2], r, repr(sf)[:60], type(sf).__name__, e.blockTableRecordId())


mlab.check(L, M, "prop_roundtrip_position_rotation_scale", br_prop)


def br_db():
    e = C()
    e.setPosition(pt(1, 1))
    return closed_loop(C, M, None, "blockref(null blockId)")


mlab.check(L, M, "db_roundtrip", br_db)


def br_readback():
    e = C()
    e.setPosition(pt(2, 2))
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid)


mlab.check(L, M, "readback_after_commit", br_readback)
mlab.probe(L, M, "ctor 字符串", lambda: C("x"), allow=("Exception",))
mlab.probe(L, M, "setScaleFactors 三 double", lambda: C().setScaleFactors(1.0, 1.0, 1.0),
           allow=("Exception",))
mlab.probe(L, M, "setRotation 传字符串", lambda: C().setRotation("abc"), allow=("Exception",))
mlab.probe(L, M, "setPosition 传 None", lambda: C().setPosition(None), allow=("Exception",))

# ================================================================ PyDbHatch
M = "PyDbHatch"
C = P.Db.PyDbHatch

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def hatch_prop():
    e = C()
    e.setOriginPoint(pt2(2.0, 3.0))
    e.setPatternScale(2.0)
    e.setPatternAngle(0.5)
    e.setAssociative(True)
    o = e.originPoint()
    ps, pa = e.patternScale(), e.patternAngle()
    assoc = e.associative()
    return "origin=%s(%s) patternScale=%.3f patternAngle=%.3f associative=%s(%s) numLoops=%s" % (
        repr(o)[:30], type(o).__name__, ps, pa, assoc, type(assoc).__name__, e.numLoops())


mlab.check(L, M, "prop_roundtrip_origin_scale_angle_assoc", hatch_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(C, M, lambda e: e.setPatternScale(1.0), "hatch"))


def hatch_readback():
    e = C()
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid)


mlab.check(L, M, "readback_after_commit", hatch_readback)
mlab.probe(L, M, "ctor 字符串", lambda: C("x"), allow=("Exception",))
mlab.probe(L, M, "setPatternScale 传字符串", lambda: C().setPatternScale("abc"), allow=("Exception",))
mlab.probe(L, M, "setOriginPoint 传 3d 点", lambda: C().setOriginPoint(pt(2, 3)), allow=("Exception",))
mlab.probe(L, M, "appendLoopPoints 空参", lambda: C().appendLoopPoints(), allow=("Exception", "False", "None"))

# ================================================================ PyDbTable
M = "PyDbTable"
C = P.Db.PyDbTable

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def tbl_prop():
    e = C()
    e.setNumRows(3)
    e.setNumColumns(2)
    e.setPosition(pt(4.0, 5.0, 0.0))
    e.setRotation(0.0)
    nr, nc = e.numRows(), e.numColumns()
    p = p3(e.position())
    return "numRows=%s numColumns=%s position=(%.3f,%.3f,%.3f) rotation=%.3f 回读类型 numRows=%s" % (
        nr, nc, p[0], p[1], p[2], e.rotation(), type(nr).__name__)


mlab.check(L, M, "prop_roundtrip_rows_cols_position", tbl_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(C, M, lambda e: e.setNumRows(2), "table"))


def tbl_readback():
    e = C()
    e.setNumColumns(2)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid)


mlab.check(L, M, "readback_after_commit", tbl_readback)
mlab.probe(L, M, "ctor 字符串", lambda: C("x"), allow=("Exception",))
mlab.probe(L, M, "setNumRows 传字符串", lambda: C().setNumRows("abc"), allow=("Exception",))
mlab.probe(L, M, "setNumRows 传 None", lambda: C().setNumRows(None), allow=("Exception",))
mlab.probe(L, M, "setPosition 传 None", lambda: C().setPosition(None), allow=("Exception",))

# ================================================================ PyDbViewport
M = "PyDbViewport"
C = P.Db.PyDbViewport

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def vp_prop():
    e = C()
    e.setCenterPoint(pt(6.0, 7.0, 0.0))
    e.setHeight(6.0)
    e.setWidth(9.0)
    e.setLensLength(50.0)
    c = p3(e.centerPoint())
    h, w, lens = e.height(), e.width(), e.lensLength()
    return "center=(%.3f,%.3f,%.3f) height=%.3f width=%.3f lensLength=%.3f 回读类型 height=%s" % (
        c[0], c[1], c[2], h, w, lens, type(h).__name__)


mlab.check(L, M, "prop_roundtrip_center_size_lens", vp_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(C, M, lambda e: e.setHeight(5.0), "viewport"))


def vp_readback():
    e = C()
    e.setWidth(8.0)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid)


mlab.check(L, M, "readback_after_commit", vp_readback)
mlab.probe(L, M, "ctor 字符串", lambda: C("x"), allow=("Exception",))
mlab.probe(L, M, "setHeight 传字符串", lambda: C().setHeight("abc"), allow=("Exception",))
mlab.probe(L, M, "setLensLength 传 None", lambda: C().setLensLength(None), allow=("Exception",))
mlab.probe(L, M, "setCenterPoint 传元组", lambda: C().setCenterPoint((1.0, 2.0)), allow=("Exception",))


def main():
    rep = mlab.report()
    dump = mlab.dump(ACCUM)
    receipt = mlab.out_path(r"C:\temp\shard_receipt.json")
    with open(receipt, "w", encoding="utf-8") as f:
        json.dump({"rows": rep}, f, ensure_ascii=False)
    return {"written": ACCUM, "dump": dump, "rows": len(rep)}


try:
    print(json.dumps(main(), ensure_ascii=False))
except Exception:
    print(json.dumps({"fatal": traceback.format_exc()}, ensure_ascii=False))
