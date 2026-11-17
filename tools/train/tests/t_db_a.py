# -*- coding: utf-8 -*-
"""Db 批次 A：Line / Arc / Text / MText / Polyline 四类断言 + 失败面。结果写 shard_db.json。"""
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


def vec(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def p3(p):
    if hasattr(p, "x"):
        return (p.x, p.y, p.z)
    return tuple(p)


# ---------------------------------------------------------------- 通用闭环
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


def readback(oid, expect_type):
    tried = []
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        try:
            o = tr.openObject(oid)
            tried.append("tr.openObject:%s" % type(o).__name__)
            h2 = o.handle()
            return "openObject 读回 type=%s handle=%s" % (type(o).__name__, h2)
        except Exception as e:
            tried.append("tr.openObject:%s: %s" % (type(e).__name__, str(e)[:120]))
    return "oid=%s 读回失败 tried=%s" % (oid, tried)


def exists_ctor(mod, cls, forms):
    assert hasattr(P.Db, mod), "P.Db 无 %s" % mod
    inst, form = mlab.ctor(cls, *forms)
    return "构造 %s type=%s.%s members=%d" % (
        form, type(inst).__module__, type(inst).__name__,
        len([x for x in dir(inst) if not x.startswith("_")]))


# ================================================================ PyDbLine
M = "PyDbLine"
C = P.Db.PyDbLine

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [(), (pt(0, 0), pt(1, 1))]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def line_prop():
    e = C()
    e.setStartPoint(pt(1.0, 2.0, 0.0))
    e.setEndPoint(pt(4.0, 5.0, 1.0))
    e.setNormal(vec(0.0, 0.0, 1.0))
    e.setColorIndex(3)
    s, t = p3(e.startPoint()), p3(e.endPoint())
    n = e.normal()
    ci = e.colorIndex()
    assert abs(s[0] - 1.0) < 1e-9 and abs(s[1] - 2.0) < 1e-9, "startPoint 不一致 %r" % (s,)
    assert abs(t[0] - 4.0) < 1e-9 and abs(t[2] - 1.0) < 1e-9, "endPoint 不一致 %r" % (t,)
    return "start->(%.3f,%.3f,%.3f) end->(%.3f,%.3f,%.3f) normal=(%.3f,%.3f,%.3f) colorIndex=%s(%s)" % (
        s[0], s[1], s[2], t[0], t[1], t[2], n.x, n.y, n.z, ci, type(ci).__name__)


mlab.check(L, M, "prop_roundtrip_points_normal_color", line_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(C, M, lambda e: (e.setStartPoint(pt(0, 0)), e.setEndPoint(pt(3, 2))), "line"))


def line_readback():
    e = C()
    e.setEndPoint(pt(2, 2))
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid, "PyDbLine")


mlab.check(L, M, "readback_after_commit", line_readback)
mlab.probe(L, M, "ctor 字符串", lambda: C("x"), allow=("Exception",))
mlab.probe(L, M, "setStartPoint 传字符串", lambda: C().setStartPoint("abc"), allow=("Exception",))
mlab.probe(L, M, "setEndPoint 传 None", lambda: C().setEndPoint(None), allow=("Exception",))
mlab.probe(L, M, "事务外 erase 瞬态对象", lambda: C().erase(), allow=("Exception", "False", "None"))

# ================================================================ PyDbArc
M = "PyDbArc"
C = P.Db.PyDbArc

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [(), (pt(0, 0), 5.0, 0.0, 1.5707963)]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def arc_prop():
    e = C()
    e.setCenter(pt(10.0, 20.0, 0.0))
    e.setRadius(33.25)
    e.setStartAngle(0.25)
    e.setEndAngle(2.0)
    c = p3(e.center())
    r = e.radius()
    sa, ea = e.startAngle(), e.endAngle()
    assert abs(float(r) - 33.25) < 1e-9, "radius %r" % r
    assert abs(c[0] - 10.0) < 1e-9 and abs(c[1] - 20.0) < 1e-9, "center %r" % (c,)
    return "center->(%.3f,%.3f,%.3f) radius %.4f start=%.3f end=%.3f 回读类型 center=%s radius=%s" % (
        c[0], c[1], c[2], r, sa, ea, type(e.center()).__name__, type(r).__name__)


mlab.check(L, M, "prop_roundtrip_center_radius_angles", arc_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(P.Db.PyDbArc, M, lambda e: e.setRadius(7.5), "arc"))


def arc_readback():
    e = P.Db.PyDbArc()
    e.setRadius(4.25)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid, "PyDbArc")


mlab.check(L, M, "readback_after_commit", arc_readback)
mlab.probe(L, M, "ctor 负半径", lambda: P.Db.PyDbArc(pt(0, 0), -5.0, 0.0, 1.0), allow=("Exception",))
mlab.probe(L, M, "setRadius 传字符串", lambda: P.Db.PyDbArc().setRadius("abc"), allow=("Exception",))
mlab.probe(L, M, "setRadius 传 None", lambda: P.Db.PyDbArc().setRadius(None), allow=("Exception",))
mlab.probe(L, M, "setCenter 传元组", lambda: P.Db.PyDbArc().setCenter((1.0, 2.0)), allow=("Exception",))

# ================================================================ PyDbText
M = "PyDbText"
C = P.Db.PyDbText

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [(), (pt(0, 0), "ABC", 2.5, 0.0)]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def text_prop():
    e = C()
    e.setTextString("HELLO-中文")
    e.setHeight(2.5)
    e.setRotation(0.3)
    e.setPosition(pt(3.0, 4.0, 0.0))
    s = e.textString()
    h = e.height()
    r = e.rotation()
    p = p3(e.position())
    assert s == "HELLO-中文", "textString %r" % s
    assert abs(float(h) - 2.5) < 1e-9, "height %r" % h
    return "text=%r height=%.3f rotation=%.3f position=(%.3f,%.3f,%.3f) 回读类型 text=%s height=%s" % (
        s, h, r, p[0], p[1], p[2], type(s).__name__, type(h).__name__)


mlab.check(L, M, "prop_roundtrip_text_height_rotation_position", text_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(P.Db.PyDbText, M, lambda e: e.setTextString("T"), "text"))


def text_readback():
    e = P.Db.PyDbText()
    e.setTextString("RB")
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid, "PyDbText")


mlab.check(L, M, "readback_after_commit", text_readback)
mlab.probe(L, M, "ctor 高度传字符串", lambda: P.Db.PyDbText(pt(0, 0), "X", "abc"), allow=("Exception",))
mlab.probe(L, M, "setHeight 传字符串", lambda: P.Db.PyDbText().setHeight("abc"), allow=("Exception",))
mlab.probe(L, M, "setTextString 传 None", lambda: P.Db.PyDbText().setTextString(None), allow=("Exception",))
mlab.probe(L, M, "setPosition 传 None", lambda: P.Db.PyDbText().setPosition(None), allow=("Exception",))

# ================================================================ PyDbMText
M = "PyDbMText"
C = P.Db.PyDbMText

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def mt_prop():
    e = C()
    e.setContents("MTEXT 内容")
    e.setHeight(3.0)
    e.setRotation(0.15)
    e.setLocation(pt(5.0, 6.0, 0.0))
    s = e.contents()
    h = e.height()
    r = e.rotation()
    p = p3(e.location())
    assert s == "MTEXT 内容", "contents %r" % s
    return "contents=%r height=%.3f rotation=%.3f location=(%.3f,%.3f,%.3f) 回读类型 contents=%s height=%s" % (
        s, h, r, p[0], p[1], p[2], type(s).__name__, type(h).__name__)


mlab.check(L, M, "prop_roundtrip_contents_height_rotation_location", mt_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(P.Db.PyDbMText, M, lambda e: e.setContents("MT"), "mtext"))


def mt_readback():
    e = P.Db.PyDbMText()
    e.setContents("RB-M")
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid, "PyDbMText")


mlab.check(L, M, "readback_after_commit", mt_readback)
mlab.probe(L, M, "ctor 字符串", lambda: P.Db.PyDbMText("x"), allow=("Exception",))
mlab.probe(L, M, "setHeight 传字符串", lambda: P.Db.PyDbMText().setHeight("abc"), allow=("Exception",))
mlab.probe(L, M, "setContents 传 None", lambda: P.Db.PyDbMText().setContents(None), allow=("Exception",))
mlab.probe(L, M, "setLocation 传 None", lambda: P.Db.PyDbMText().setLocation(None), allow=("Exception",))

# ================================================================ PyDbPolyline
M = "PyDbPolyline"
C = P.Db.PyDbPolyline

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def pl_prop():
    e = C()
    r1 = e.addVertexAt(0, pt2(1.0, 1.0))
    r2 = e.addVertexAt(1, pt2(3.0, 4.0))
    e.setConstantWidth(0.25)
    e.setClosed(True)
    n = e.numVerts()
    w = e.constantWidth()
    p0 = e.getPointAt(0) if hasattr(e, "getPointAt") else None
    return "addVertexAt rc=%s,%s numVerts=%s constantWidth=%.3f closed=%s p0=%s(%s) 回读类型 numVerts=%s" % (
        r1, r2, n, w, e.isClosed(), repr(p0)[:40], type(p0).__name__, type(n).__name__)


mlab.check(L, M, "prop_roundtrip_vertices_width_closed", pl_prop)
mlab.check(L, M, "db_roundtrip", lambda: closed_loop(P.Db.PyDbPolyline, M, lambda e: e.addVertexAt(0, pt2(0, 0)), "pline"))


def pl_readback():
    e = P.Db.PyDbPolyline()
    e.addVertexAt(0, pt2(1, 1))
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    return readback(oid, "PyDbPolyline")


mlab.check(L, M, "readback_after_commit", pl_readback)
mlab.probe(L, M, "ctor 字符串", lambda: P.Db.PyDbPolyline("x"), allow=("Exception",))
mlab.probe(L, M, "addVertexAt 越界索引", lambda: P.Db.PyDbPolyline().addVertexAt(999, pt2(1, 1)),
           allow=("Exception", "False", "None"))
mlab.probe(L, M, "setPointAt 越界索引", lambda: P.Db.PyDbPolyline().setPointAt(999, pt2(1, 1)),
           allow=("Exception", "False", "None"))
mlab.probe(L, M, "addVertexAt 点传字符串", lambda: P.Db.PyDbPolyline().addVertexAt(0, "abc"),
           allow=("Exception",))


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
