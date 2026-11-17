# -*- coding: utf-8 -*-
"""PyArx batch7：真机全功能回归（绘图/修改/删除/图层/块表/迭代协议/Ed）。

每步独立落盘，卡住时最后一个 step 即卡点。
结果：G:\PyArx\log\pyarx_api_test_batch7.json   图形：G:\PyArx\log\pyarx_batch7.dwg
"""
import json
import math
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch7.json"
SAVE = r"G:\PyArx\log\pyarx_batch7.dwg"

db = P.Db.curDb()
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d
P2 = P.Ge.AcGePoint2d

R = {"steps": [], "done": False}
IDS = {}


# ============================ 工具 ============================
def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def step(k, fn):
    rec = {"step": k, "ok": False, "err": None, "tb": None, "data": None}
    try:
        out = fn()
        rec["ok"] = True
        rec["data"] = out
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:400]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:260]
    R["steps"].append(rec)
    flush()
    return rec


def ent(k, maker):
    with P.Db.PyDbTrans() as tr:
        e = maker()
        ok = tr.addEntity(e)
        oid = e.objectId()
        h = e.handle()
    IDS[k] = oid
    return {"add": ok, "id": oid, "handle": h}


def open_obj(oid, mode=1):
    return db.openObject(oid, mode)


# ============================ 绘图形状 ============================
def _pline():
    pl = P.Db.PyDbPolyline()
    pl.addVertexAt(0, P2(200, 0), 0.0, -1.0, -1.0)
    pl.addVertexAt(1, P2(240, 40), 1.0, -1.0, -1.0)
    pl.addVertexAt(2, P2(280, 0), 0.0, -1.0, -1.0)
    pl.setWidthsAt(2, 2.0, 2.0)
    pl.setClosed(False)
    return pl


def _mtext():
    t = P.Db.PyDbMText()
    t.setLocation(P3(0, -40, 0))
    t.setContents("PyArx 多行文字\\P第二行")
    t.setWidth(80.0)
    t.setHeight(5.0)
    t.setAttachment(1)
    return t


def _spline():
    pts = [P3(300, 0, 0), P3(330, 30, 0), P3(360, 0, 0), P3(390, 30, 0)]
    return P.Db.PyDbSpline(pts, V3(1, 0, 0), V3(1, 0, 0), 0, 3)


def _ray():
    r = P.Db.PyDbRay()
    r.setBasePoint(P3(0, 150, 0))
    r.setUnitDir(V3(0, 1, 0))
    return r


def _xline():
    x = P.Db.PyDbXline()
    x.setBasePoint(P3(0, 180, 0))
    x.setUnitDir(V3(1, 0, 0))
    return x


def _solid3d():
    s = P.Db.PyDb3dSolid()
    s.createBox(P3(420, 0, 0), 40.0, 40.0, 40.0)
    return s


# ============================ C 读回 ============================
def _readback():
    out = {}
    with P.Db.PyDbTrans() as tr:
        for k, oid in IDS.items():
            try:
                e = tr.openObject(oid, 0)
                rec = {"class": e.className()}
                ext = e.getGeomExtents() if hasattr(e, "getGeomExtents") else None
                rec["ext"] = None if ext is None else [str(ext[0]), str(ext[1])]
                out[k] = rec
            except Exception as ex:
                out[k] = {"err": type(ex).__name__ + ": " + str(ex)[:140]}
    c = open_obj(IDS["circle"], 0)
    out["circle_radius"] = c.radius()
    c.close()
    ln = open_obj(IDS["line"], 0)
    out["line_end"] = [ln.endPoint().x, ln.endPoint().y, ln.endPoint().z]
    ln.close()
    pl = open_obj(IDS["pline"], 1)
    out["pline_numVerts"] = pl.numVerts()
    out["pline_bulge1"] = pl.getBulgeAt(1)
    pl.close()
    return out


# ============================ D 修改 ============================
def _modify():
    out = {}
    e = open_obj(IDS["line"], 1)
    out["setColor"] = e.setColorIndex(3)
    out["color"] = e.colorIndex()
    out["setLinetype"] = e.setLinetype("CONTINUOUS")
    e.close()

    c = open_obj(IDS["circle"], 1)
    out["setRadius"] = c.setRadius(35.0)
    out["radius"] = c.radius()
    c.close()

    t = open_obj(IDS["text"], 1)
    out["setText"] = t.setTextString("PyArx T7 modified")
    out["text"] = t.textString()
    out["setHeight"] = t.setHeight(6.0)
    out["height"] = t.height()
    t.close()

    pl = open_obj(IDS["pline"], 1)
    out["setPointAt"] = pl.setPointAt(0, P2(210, 10))
    pl.close()

    m = P.Ge.AcGeMatrix3d()
    arc = open_obj(IDS["arc"], 1)
    try:
        m.setToRotation(0.3, V3(0, 0, 1), P3(0, 0, 0))
        out["transformBy"] = arc.transformBy(m)
        out["arc_center"] = str(arc.center())
    except Exception as ex:
        out["transform_err"] = type(ex).__name__ + ": " + str(ex)[:150]
    arc.close()
    out["circle_circumference"] = None
    return out


# ============================ E 图层 ============================
def _layers():
    out = {}
    lt = db.getLayerTable()
    rec = P.Db.PyDbLayerTableRecord.newRecord()
    out["setName"] = rec.setName("PYX_T7_LAYER")
    rec.setColor(200, 80, 40)
    rec.setIsOff(False)
    rec.setIsLocked(False)
    out["add"] = lt.add(rec)
    IDS["layer"] = rec.objectId()
    out["id"] = IDS["layer"]
    out["has"] = lt.has("PYX_T7_LAYER")

    got = lt.getAt("PYX_T7_LAYER", 0)
    out["getAt_name"] = str(got.getName())
    out["getAt_color"] = str(got.color())
    got.close()

    out["keys_n"] = len(lt.keys())
    out["values_n"] = len(lt.values(0))
    out["items_n"] = len(lt.items(0))
    out["toDict_n"] = len(lt.toDict())
    out["numRecords"] = lt.numRecords()

    e = open_obj(IDS["line"], 1)
    out["setLayer"] = e.setLayer("PYX_T7_LAYER")
    out["layer_now"] = str(e.layer())
    e.close()

    lr = lt.getAt("PYX_T7_LAYER", 1)
    lr.setIsOff(True)
    out["off"] = lr.isOff()
    lr.setIsOff(False)
    lr.setIsLocked(True)
    out["locked"] = lr.isLocked()
    lr.setIsLocked(False)
    lr.close()
    out["clayer"] = str(db.clayer())
    out["layer_zero"] = str(db.layerZero())
    return out


# ============================ F 块表 ============================
def _blocks():
    out = {}
    blk_id = 0
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T7_BLOCK")
        c = P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 10)
        btr.appendAcDbEntity(c)
        ln = P.Db.PyDbLine(P3(-10, 0, 0), P3(10, 0, 0))
        btr.appendAcDbEntity(ln)
        out["add_block"] = bt.add(btr)
        blk_id = btr.objectId()
        out["block_id"] = blk_id
        out["numEntities"] = btr.numEntities()
        out["entityIds"] = len(btr.entityIds())
        out["len_btr"] = len(btr)

        refs = []
        for i, pos in enumerate((P3(500, 0, 0), P3(560, 0, 0)), 1):
            br = P.Db.PyDbBlockReference()
            out["setBTR_%d" % i] = br.setBlockTableRecord(blk_id)
            out["setPos_%d" % i] = br.setPosition(pos)
            out["setRot_%d" % i] = br.setRotation(0.2 * i)
            tr.addEntity(br)
            refs.append(br.objectId())
        out["blkref_ids"] = refs

    mspace = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    out["space_len"] = len(mspace)
    out["space_numEntities"] = mspace.numEntities()
    mspace.close()

    br = open_obj(blk_id, 0)  # 块定义本体可读打开
    out["block_name"] = str(br.getName())
    br.close()
    return out


# ============================ G 迭代协议 ============================
def _iter_proto():
    out = {}
    btr = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
    out["len"] = len(btr)
    ids = btr.entityIds()
    out["ids_n"] = len(ids)
    out["values_n"] = len(btr.values(1))
    out["items_n"] = len(btr.items(1))
    out["keys_n"] = len(btr.keys())

    first = ids[0]
    out["getitem"] = not btr[first].isNull()
    out["get"] = btr.get(first, None) is not None
    out["get_missing"] = btr.get(999999999, None) is None
    out["contains"] = first in btr

    n = 0
    for e in btr:
        e.setColorIndex(2)
        n += 1
    out["iter_n"] = n
    btr.close()

    lt = db.getLayerTable()
    out["layer_values_n"] = len(lt.values(0))
    zero = lt["0"]
    out["layer_zero"] = not zero.isNull()
    zero.close()
    return out


# ============================ H Ed ============================
def _ed():
    out = {}
    h = open_obj(IDS["line"], 0)
    handle = str(h.handle())
    h.close()
    out["handle"] = handle
    en = P.Ed.handent(handle.upper())
    out["handent"] = en is not None
    if en is not None:
        dxf = P.Ed.entget(en)
        out["entget"] = None if dxf is None else {"n": len(dxf), "0": dxf.get(0), "8": dxf.get(8), "62": dxf.get(62)}
    out["entlast"] = P.Ed.entlast() is not None
    out["sssetfirst"] = P.Ed.sssetfirst([IDS["line"], IDS["circle"]])
    g1, g2 = P.Ed.ssgetfirst()
    out["ssgetfirst"] = [len(g1), len(g2)]
    out["sssetfirst_clear"] = P.Ed.sssetfirst(None)
    try:
        ss = P.Ed.ssget("X", None, None, [(0, "CIRCLE")])
        out["ssget_filter"] = None if ss is None else P.Ed.sslength(ss)
    except Exception as ex:
        out["ssget_err"] = type(ex).__name__ + ": " + str(ex)[:150]
    return out


# ============================ I 删除 ============================
def _delete():
    out = {}
    n0 = len(db.objectIds("AcDbEntity"))
    e = open_obj(IDS["point"], 1)
    out["erase"] = e.erase()
    out["isErased"] = e.isErased()
    e.close()
    n1 = len(db.objectIds("AcDbEntity"))
    e = open_obj(IDS["point"], 1)
    out["unerase"] = e.erase(False)
    e.close()
    n2 = len(db.objectIds("AcDbEntity"))
    out["count"] = {"before": n0, "erased": n1, "restored": n2}
    return out


# ============================ J 保存 ============================
def _save():
    if os.path.exists(SAVE):
        os.remove(SAVE)
    st = db.saveAs(SAVE)
    return {"status": str(st), "exists": os.path.exists(SAVE),
            "entities": len(db.objectIds("AcDbEntity"))}


# ============================ 执行 ============================
flush()
step("A_env", lambda: {
    "file": str(db.getFilename()),
    "entities": len(db.objectIds("AcDbEntity")),
    "space": db.currentSpaceId(),
    "clayer": str(db.clayer()),
})
step("B_line", lambda: ent("line", lambda: P.Db.PyDbLine(P3(0, 0, 0), P3(100, 0, 0))))
step("B_circle", lambda: ent("circle", lambda: P.Db.PyDbCircle(P3(50, 50, 0), V3(0, 0, 1), 20)))
step("B_arc", lambda: ent("arc", lambda: P.Db.PyDbArc(P3(0, 60, 0), 30, 0.0, math.pi)))
step("B_pline", lambda: ent("pline", _pline))
step("B_text", lambda: ent("text", lambda: P.Db.PyDbText(P3(0, -20, 0), "PyArx T7", 4.0, 0.0)))
step("B_mtext", lambda: ent("mtext", _mtext))
step("B_ellipse", lambda: ent("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50, 0), V3(0, 0, 1), V3(25, 0, 0), 0.4)))
step("B_point", lambda: ent("point", lambda: P.Db.PyDbPoint(P3(150, 0, 0))))
step("B_solid", lambda: ent("solid", lambda: P.Db.PyDbSolid(P3(0, 100, 0), P3(20, 100, 0), P3(20, 120, 0), P3(0, 120, 0))))
step("B_ray", lambda: ent("ray", _ray))
step("B_xline", lambda: ent("xline", _xline))
step("B_spline", lambda: ent("spline", _spline))
step("B_3dsolid", lambda: ent("solid3d", _solid3d))
step("C_readback", _readback)
step("D_modify", _modify)
step("E_layers", _layers)
step("F_blocks", _blocks)
step("G_iter_proto", _iter_proto)
step("H_ed", _ed)
step("I_delete", _delete)
step("J_save", _save)

R["done"] = True
flush()
print("[PyArx] batch7 done")
