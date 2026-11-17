# -*- coding: utf-8 -*-
"""补丁：PyDbEntity / PyDbObject / PyDbCurve / PyDbLayerTableRecord 补属性往返 + 入库闭环。

实测结论（本轮诊断）：
- 基类构造器 init<AcDbEntity*/AcDbObject*/AcDbCurve*> 同时拒绝具体 wrapper 与 intPtr，
  无法把 openObject 返回的具体实体重新包装成基类实例；但 Python 继承下具体 wrapper
  已直接暴露基类成员（colorIndex/layer/handle/objectId 等），属性往返在具体 wrapper 上完成。
- PyDbLayerTableRecord：lt.getAt("0") 直接返回记录 wrapper，不再二次包装。
"""
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


def make_line():
    return P.Db.PyDbLine(pt(0, 0, 0), pt(10, 0, 0))


def closed_loop(tag):
    db = P.Db.curDb()
    before = mlab.ent_count()
    e = make_line()
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        h = str(e.handle())
        oid = e.objectId()
    after = mlab.ent_count()
    assert h != "0", "句柄为 0"
    assert after > before, "count %d->%d" % (before, after)
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(oid)
    return "%s before=%d after=%d handle=%s objectId=%s 回读wrapper=%s" % (
        tag, before, after, h, oid, type(o).__name__)


def readback():
    e = make_line()
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        oid = e.objectId()
        h = str(e.handle())
    with P.Db.PyDbTrans() as tr:
        o = tr.openObject(oid)
        return "openObject 回读 wrapper=%s handle=%s" % (type(o).__name__, str(o.handle()))


# ================================================================ PyDbEntity
M = "PyDbEntity"
C = P.Db.PyDbEntity

mlab.check(L, M, "exists+closed_loop_on_concrete_wrapper", lambda: closed_loop("entity"))
mlab.check(L, M, "readback_after_commit", readback)


def ent_prop():
    e = make_line()
    e.setColorIndex(5)
    ci = e.colorIndex()
    layer = e.layer()
    h = str(e.handle())
    oid = e.objectId()
    assert int(ci) == 5, "colorIndex 读回 %r" % ci
    return "colorIndex=%s(%s) layer=%r handle=%s objectId=%s(%s) 基类成员由具体wrapper继承" % (
        ci, type(ci).__name__, layer, h, oid, type(oid).__name__)


mlab.check(L, M, "prop_roundtrip_colorIndex_layer_handle", ent_prop)
mlab.probe(L, M, "包装非实体字符串", lambda: C("not-an-entity"), allow=("Exception",))

# ================================================================ PyDbObject
M = "PyDbObject"
C = P.Db.PyDbObject

mlab.check(L, M, "exists+closed_loop_on_concrete_wrapper", lambda: closed_loop("object"))
mlab.check(L, M, "readback_after_commit", readback)


def obj_prop():
    e = make_line()
    h = str(e.handle())
    oid = e.objectId()
    owner = e.ownerId()
    return "handle=%s objectId=%s(%s) ownerId=%s(%s) 基类无颜色/层 setter" % (
        h, oid, type(oid).__name__, owner, type(owner).__name__)


mlab.check(L, M, "prop_roundtrip_handle_objectId_ownerId", obj_prop)
mlab.probe(L, M, "包装非实体字符串", lambda: C("not-an-entity"), allow=("Exception",))

# ================================================================ PyDbCurve
M = "PyDbCurve"
C = P.Db.PyDbCurve

mlab.check(L, M, "exists+closed_loop_on_concrete_wrapper", lambda: closed_loop("curve"))
mlab.check(L, M, "readback_after_commit", readback)


def curve_prop():
    e = make_line()
    e.setColorIndex(3)
    ci = e.colorIndex()
    layer = e.layer()
    sp = e.getStartPoint()
    ep = e.getEndPoint()
    return "colorIndex=%s layer=%r start=(%.3f,%.3f,%.3f) end=(%.3f,%.3f,%.3f)" % (
        ci, layer, sp.x, sp.y, sp.z, ep.x, ep.y, ep.z)


mlab.check(L, M, "prop_roundtrip_colorIndex_layer_endpoints", curve_prop)
mlab.probe(L, M, "包装非曲线实体(MText)", lambda: C(P.Db.PyDbMText()), allow=("Exception",))

# ================================================================ PyDbLayerTableRecord
M = "PyDbLayerTableRecord"
C = P.Db.PyDbLayerTableRecord
_LTR_DB = P.Db.curDb()
_LTR_LT = _LTR_DB.getLayerTable()
_LTR_REC = _LTR_LT.getAt("0")


def ltr_open():
    rec = _LTR_REC
    return "layerTable=%s getAt(0)->%s 包装=%s" % (
        type(_LTR_LT).__name__, type(rec).__name__, type(rec).__name__)


mlab.check(L, M, "exists+open_existing_layer", ltr_open)


def ltr_prop():
    rec = _LTR_REC
    nm = rec.getName()
    col = rec.color()
    frozen = rec.isFrozen()
    hidden = rec.isHidden()
    oid = rec.objectId()
    orig_frozen = frozen
    rec.setIsFrozen(True)
    after = rec.isFrozen()
    rec.setIsFrozen(orig_frozen)  # 恢复原值
    return "name=%r color=%s(%s) frozen=%s->%s(已恢复) hidden=%s objectId=%s" % (
        nm, col, type(col).__name__, frozen, after, hidden, oid)


mlab.check(L, M, "prop_roundtrip_name_color_frozen_hidden", ltr_prop)


def ltr_closed_loop():
    rec = _LTR_REC
    before = mlab.ent_count()
    e = make_line()
    e.setLayerId(rec.objectId())
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
        h = str(e.handle())
    after = mlab.ent_count()
    layer_name = e.layer()
    return "线设到层记录 objectId=%s 回读 layer=%r before=%d after=%d handle=%s" % (
        rec.objectId(), layer_name, before, after, h)


mlab.check(L, M, "closed_loop_set_line_layer_by_record", ltr_closed_loop)
mlab.probe(L, M, "包装非图层记录字符串", lambda: C("not-a-layer"), allow=("Exception",))


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
