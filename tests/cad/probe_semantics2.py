# -*- coding: utf-8 -*-
"""语义探针 2：块遍历/选择集/删除/保存。"""
import json
import os

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_probe2.json"
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


flush()

ID = {"line": 0}
with P.Db.PyDbTrans() as tr:
    ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
    tr.addEntity(ln)
    ID["line"] = ln.objectId()


def p_iter_entities():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    try:
        out = {}
        for mode in (0, 1):
            try:
                it = btr.iterEntities(mode, True)
                n = 0
                cls = []
                for e in it:
                    n += 1
                    cls.append(str(e.className()))
                out["iterEntities_%d" % mode] = {"n": n, "cls": cls[:3]}
            except Exception as ex:
                out["iterEntities_%d" % mode] = "ERR " + type(ex).__name__ + ":" + str(ex)[:80]
        try:
            items = btr.items(1)
            out["items_n"] = len(items)
            out["items0_type"] = type(items[0]).__name__ if items else None
        except Exception as ex:
            out["items"] = "ERR " + str(ex)[:80]
        try:
            keys = btr.keys()
            out["keys_n"] = len(keys)
        except Exception as ex:
            out["keys"] = "ERR " + str(ex)[:80]
        try:
            e = btr.get(ID["line"], None)
            out["get"] = None if e is None else (type(e).__name__, e.isNull())
        except Exception as ex:
            out["get"] = "ERR " + str(ex)[:80]
        try:
            out["contains"] = ID["line"] in btr
        except Exception as ex:
            out["contains"] = "ERR " + str(ex)[:80]
        return out
    finally:
        btr.close()


probe("btr_iter_items", p_iter_entities)


def p_getentity():
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
    try:
        e = btr.getEntityById(ID["line"], 0)
        try:
            return {"type": type(e).__name__, "class": str(e.className()),
                    "end": [e.endPoint().x, e.endPoint().y]}
        finally:
            e.close()
    finally:
        btr.close()


probe("getEntityById", p_getentity)


def p_ssget_entget():
    ss = P.Ed.ssget("X", None, None, [(0, "LINE")])
    n = P.Ed.sslength(ss)
    en = P.Ed.ssname(ss, 0)
    dxf = P.Ed.entget(en)
    return {"n": n, "ename_type": type(en).__name__,
            "entget_type": type(dxf).__name__,
            "codes": sorted([int(k) for k in dxf.keys()]) if isinstance(dxf, dict) else None,
            "layer": dxf.get(8) if isinstance(dxf, dict) else None}


probe("ssname_entget", p_ssget_entget)


def p_sssetfirst():
    ok = P.Ed.sssetfirst([ID["line"]])
    g = P.Ed.ssgetfirst()
    clr = P.Ed.sssetfirst([])
    return {"set": ok, "first_n": len(g[1]), "clear": clr}


probe("sssetfirst_empty", p_sssetfirst)


def p_block():
    out = {}
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_PROBE_BLK")
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 5)
        btr.appendAcDbEntity(c)
        out["bt_add"] = bt.add(btr)
        blk = btr.objectId()
        br = P.Db.PyDbBlockReference()
        out["setBTR"] = br.setBlockTableRecord(blk)
        out["setPos"] = br.setPosition(P3(300, 0, 0))
        out["addRef"] = tr.addEntity(br)
        out["ref_id"] = br.objectId()
    return out


probe("block_insert", p_block)


def p_layer_delete():
    out = {}
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName("PYX_PROBE_DEL")
        out["add"] = lt.add(rec)
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable(0)
        r = lt.getAt("PYX_PROBE_DEL")
        out["erase"] = r.erase()
        out["isErased"] = r.isErased()
        r.close()
        lt.close()
    return out


probe("layer_delete", p_layer_delete)


def p_erase_entity():
    out = {}
    n0 = len(db.objectIds("AcDbEntity"))
    with P.Db.PyDbTrans() as tr:
        e = tr.openObject(ID["line"], 1)
        out["erase"] = e.erase()
        out["isErased"] = e.isErased()
    n1 = len(db.objectIds("AcDbEntity"))
    with P.Db.PyDbTrans() as tr:
        e = tr.openObject(ID["line"], 1)
        out["unerase"] = e.erase(False)
    out["n"] = [n0, n1]
    return out


probe("erase_entity", p_erase_entity)


def p_save():
    path = r"G:\PyArx\log\pyarx_probe2.dwg"
    if os.path.exists(path):
        os.remove(path)
    st = db.saveAs(path)
    return {"status": str(st), "exists": os.path.exists(path)}


probe("db_saveAs", p_save)

R["done"] = True
flush()
print("[PyArx] probe2 done")
