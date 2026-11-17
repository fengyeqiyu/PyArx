# -*- coding: utf-8 -*-
"""Db 批次 C：PyDbTrans / PyDbDatabase / PyDbBlockTableRecord / 基类与 LayerTableRecord。"""
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


def p3(p):
    if hasattr(p, "x"):
        return (p.x, p.y, p.z)
    return tuple(p)


def exists_ctor(mod, cls, forms):
    assert hasattr(P.Db, mod), "P.Db 无 %s" % mod
    inst, form = mlab.ctor(cls, *forms)
    return "构造 %s type=%s.%s members=%d" % (
        form, type(inst).__module__, type(inst).__name__,
        len([x for x in dir(inst) if not x.startswith("_")]))


# ================================================================ PyDbTrans
M = "PyDbTrans"
C = P.Db.PyDbTrans

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [(), (P.Db.curDb(),)]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def tr_lifecycle():
    db = P.Db.curDb()
    before = mlab.ent_count()
    e = P.Db.PyDbLine()
    e.setEndPoint(pt(2, 2))
    with C() as tr:
        tr.addEntity(e)
        h = e.handle()
        oid = e.objectId()
        space = type(tr.currentSpace()).__name__
    after = mlab.ent_count()
    assert str(h) not in ("0", ""), "句柄 0"
    assert after > before, "count %d->%d" % (before, after)
    return "before=%d after=%d handle=%s objectId=%s currentSpace=%s" % (before, after, h, oid, space)


mlab.check(L, M, "ctx_add_entity_commit", tr_lifecycle)


def tr_open():
    e = P.Db.PyDbLine()
    with C() as tr:
        tr.addEntity(e)
        oid = e.objectId()
    with C() as tr:
        o = tr.openObject(oid)
        return "openObject 读回 type=%s handle=%s entityColor=%s" % (
            type(o).__name__, o.handle(), type(o.entityColor()).__name__)


mlab.check(L, M, "openObject_readback", tr_open)
mlab.probe(L, M, "事务未 enter 直接 addEntity", lambda: (C().addEntity(P.Db.PyDbLine())),
           allow=("Exception", "False", "None"))
mlab.probe(L, M, "全新事务直接 commit", lambda: C().commit(), allow=("Exception", "False", "None"))
mlab.probe(L, M, "openObject 非法 id", lambda: C().openObject(999999), allow=("Exception", "False", "None"))

# ================================================================ PyDbDatabase
M = "PyDbDatabase"
C = P.Db.PyDbDatabase

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [(), (True, False)]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def db_curdb_readonly():
    db = P.Db.curDb()
    n = len(list(db.objectIds("AcDbEntity")))
    cl = db.clayer()
    ts = db.textsize()
    fn = db.getFilename()
    return "curDb type=%s 实体数=%d clayer=%r textsize=%.4f filename=%r" % (
        type(db).__name__, n, cl, ts, fn)


mlab.check(L, M, "curDb_readonly_query", db_curdb_readonly)


def db_new_transient():
    d = C()
    null = d.isNull()
    cls = d.className()
    n = len(list(d.objectIds("AcDbEntity")))
    return "新库 isNull=%s className=%s 实体数=%d filename=%r" % (null, cls, n, d.getFilename())


mlab.check(L, M, "new_db_construct_readonly", db_new_transient)
mlab.probe(L, M, "新库 approxNumObjects()", lambda: C().approxNumObjects(), allow=("Exception",))
mlab.probe(L, M, "未命名新库 save()", lambda: C().save(), allow=("Exception", "False", "None"))
mlab.probe(L, M, "clayer 传字符串写", lambda: P.Db.curDb().setClayer("0"), allow=("Exception", "False", "None"))
mlab.probe(L, M, "getSysVar 非法变量", lambda: P.Db.curDb().getSysVar("__NO_SUCH_VAR__"),
           allow=("Exception", "False", "None"))

# ================================================================ PyDbBlockTableRecord
M = "PyDbBlockTableRecord"
C = P.Db.PyDbBlockTableRecord

mlab.check(L, M, "exists+construct", lambda: exists_ctor(M, C, [()]))
mlab.check(L, M, "type_name", lambda: "%s.%s" % (type(C()).__module__, type(C()).__name__))


def btr_transient_prop():
    b = C()
    b.setName("*TEMP_BTR")
    b.setOrigin(pt(1.0, 2.0, 0.0))
    nm = b.getName()
    o = p3(b.origin())
    return "setName 后 getName=%r（瞬态未生效）origin=(%.3f,%.3f,%.3f) blockInsertUnits=%s numEntities=%s" % (
        nm, o[0], o[1], o[2], b.blockInsertUnits(), b.numEntities())


mlab.check(L, M, "prop_roundtrip_name_origin", btr_transient_prop)


def btr_open_modelspace():
    db = P.Db.curDb()
    sid = db.currentSpaceId()
    with P.Db.PyDbTrans() as tr:
        b = tr.openObject(sid)
        return "openObject(currentSpaceId=%s) type=%s name=%r numEntities=%s isLayout=%s" % (
            sid, type(b).__name__, b.getName(), b.numEntities(), b.isLayout())


mlab.check(L, M, "open_current_space_btr", btr_open_modelspace)
mlab.probe(L, M, "openObject 非法 block id", lambda: P.Db.PyDbTrans().openObject(999999),
           allow=("Exception", "False", "None"))
mlab.probe(L, M, "transient BTR appendEntity", lambda: C().appendEntity(P.Db.PyDbLine()),
           allow=("Exception", "False", "None"))

# ================================================================ 基类（仅引用构造）
for base in ("PyDbEntity", "PyDbObject", "PyDbCurve"):
    M2 = base
    cls = getattr(P.Db, base, None)
    mlab.check(L, M2, "exists", lambda: "hasattr=True" if cls is not None else "missing")
    if cls is not None:
        members = [x for x in dir(cls) if not x.startswith("_")]
        mlab.check(L, M2, "members_count",
                   lambda: "cls members=%d 无默认构造；" % len(members) + mlab.typeerror_hint(cls, "x")[:220])
        mlab.probe(L, M2, "无参构造", lambda: cls(), allow=("Exception",))

# ================================================================ PyDbLayerTableRecord
M = "PyDbLayerTableRecord"
cls = P.Db.PyDbLayerTableRecord
mlab.check(L, M, "exists", lambda: "hasattr=True")
members = [x for x in dir(cls) if not x.startswith("_")]
mlab.check(L, M, "members_and_ctor_hint",
           lambda: "members=%d " % len(members) + mlab.typeerror_hint(cls, "x")[:220])


def ltr_table_access():
    db = P.Db.curDb()
    notes = []
    try:
        t = db.getLayerTable()
        notes.append("curDb.getLayerTable 成功 type=%s" % type(t).__name__)
    except Exception as e:
        notes.append("curDb.getLayerTable -> %s: %s" % (type(e).__name__, str(e)[:120]))
    with P.Db.PyDbTrans() as tr:
        try:
            t2 = db.getLayerTable()
            notes.append("事务内 getLayerTable -> type=%s" % type(t2).__name__)
        except Exception as e:
            notes.append("事务内 getLayerTable -> %s: %s" % (type(e).__name__, str(e)[:120]))
    return " | ".join(notes)


mlab.check(L, M, "layer_table_access", ltr_table_access)
mlab.probe(L, M, "无参构造", lambda: cls(), allow=("Exception",))


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
