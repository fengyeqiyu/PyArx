# -*- coding: utf-8 -*-
"""语义探针 3：属性访问/删除/块参照插入/保存路径/映射协议。"""
import json
import os

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_probe3.json"
R = {"probes": [], "done": False}
db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d

IDS = {}


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


flush()

# 干净起点：新建两条线
with P.Db.PyDbTrans() as tr:
    a = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
    tr.addEntity(a)
    IDS["a"] = a.objectId()
    b = P.Db.PyDbLine(P3(0, 5, 0), P3(10, 5, 0))
    tr.addEntity(b)
    IDS["b"] = b.objectId()
flush()

R["ids"] = IDS
flush()


def p_open_typed():
    out = {}
    o = db.openObject(IDS["a"], 0)
    out["open_type"] = type(o).__name__
    out["isNull"] = o.isNull()
    ln = P.Db.PyDbLine.cast(o)
    out["cast"] = type(ln).__name__
    out["end"] = [ln.endPoint().x, ln.endPoint().y]
    out["upgrade"] = ln.upgradeOpen()
    out["setColor"] = ln.setColorIndex(5)
    out["color"] = ln.colorIndex()
    ln.close()
    return out


probe("open_typed_modify", p_open_typed)


def p_erase_via_open():
    out = {}
    n0 = len(db.objectIds("AcDbEntity"))
    o = db.openObject(IDS["b"], 1)
    out["open1_type"] = type(o).__name__ if o is not None else None
    if o is None:
        return out
    out["erase"] = o.erase()
    out["erased"] = o.isErased()
    o.close()
    n1 = len(db.objectIds("AcDbEntity"))
    o2 = db.openObject(IDS["b"], 1)
    out["unerase"] = o2.erase(False) if o2 is not None else None
    if o2 is not None:
        o2.close()
    out["n"] = [n0, n1, len(db.objectIds("AcDbEntity"))]
    return out


probe("erase_via_openObject", p_erase_via_open)


# 块定义 + 块参照（三种写法）
def p_block_def():
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_P3_BLK")
        btr.appendAcDbEntity(P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 5))
        ok = bt.add(btr)
        IDS["blk"] = btr.objectId()
    return {"add": ok, "blk": IDS["blk"]}


probe("block_def", p_block_def)


def p_ref_v1():
    # 事务内 addEntity
    with P.Db.PyDbTrans() as tr:
        br = P.Db.PyDbBlockReference()
        r1 = br.setBlockTableRecord(IDS["blk"])
        r2 = br.setPosition(P3(100, 0, 0))
        add = tr.addEntity(br)
        return {"setBTR": r1, "setPos": r2, "add": add, "id": br.objectId()}


probe("blockref_v1_addEntity", p_ref_v1)


def p_ref_v2():
    # 显式 append 到模型空间块记录
    out = {}
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
        br = P.Db.PyDbBlockReference()
        out["setBTR"] = br.setBlockTableRecord(IDS["blk"])
        out["setPos"] = br.setPosition(P3(140, 0, 0))
        out["append"] = sp.appendAcDbEntity(br)
        out["append2"] = sp.appendEntity(br)
        out["id"] = br.objectId()
        sp.close()
    return out


probe("blockref_v2_append", p_ref_v2)


def p_ref_v3():
    # 直接 new AcDbBlockReference + 手动关闭
    out = {}
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
        br = P.Db.PyDbBlockReference()
        out["setBTR"] = br.setBlockTableRecord(IDS["blk"])
        br.setPosition(P3(180, 0, 0))
        out["appendAcDbEntityId"] = sp.appendAcDbEntityId(IDS["blk"])
        sp.close()
    return out


probe("blockref_v3_appendId", p_ref_v3)


def p_mapping():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
    try:
        ids = btr.entityIds()
        out = {"ids": len(ids), "values": len(btr.values(0)), "items": len(btr.items(0)),
               "keys": len(btr.keys()), "num": btr.numEntities(), "len": len(btr)}
        if ids:
            e = btr.getEntityById(ids[0], 0)
            out["getEntity_type"] = type(e).__name__
            out["getEntity_class"] = str(e.className())
            ln = P.Db.PyDbLine.cast(e)
            out["cast_ok"] = not ln.isNull()
            e.close()
            item = btr[ids[0]]
            out["getitem_null"] = item.isNull()
        return out
    finally:
        btr.close()


probe("mapping_protocol", p_mapping)


def p_save_paths():
    out = {}
    cands = [r"G:\PyArx\log\pyarx_p3_1.dwg", r"G:\PyArx\pyarx_p3_2.dwg",
             os.path.join(os.environ.get("TEMP", r"C:\Temp"), "pyarx_p3_3.dwg")]
    for p in cands:
        try:
            if os.path.exists(p):
                os.remove(p)
        except Exception:
            pass
        try:
            st = str(db.saveAs(p))
        except Exception as ex:
            st = "EXC " + type(ex).__name__ + ":" + str(ex)[:80]
        out[p] = {"status": st, "exists": os.path.exists(p)}
    return out


probe("saveAs_paths", p_save_paths)

R["done"] = True
flush()
print("[PyArx] probe3 done")
