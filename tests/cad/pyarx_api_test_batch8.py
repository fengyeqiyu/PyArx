# -*- coding: utf-8 -*-
"""PyArx batch8：干净进程下的全功能回归（事务内访问，避免开对象泄漏）。

结果：G:\PyArx\log\pyarx_api_test_batch8.json   图形：G:\PyArx\log\pyarx_batch8.dwg
"""
import json
import math
import os
import traceback

import PyArxCore as P

OUT = r"G:\PyArx\log\pyarx_api_test_batch8.json"
SAVE = r"G:\PyArx\log\pyarx_batch8.dwg"

db = P.Db.curDb()
doc = P.Ap.PyApp.documentManager.curDocument
P3 = P.Ge.AcGePoint3d
V3 = P.Ge.AcGeVector3d
P2 = P.Ge.AcGePoint2d

R = {"steps": [], "done": False}
IDS = {}


def flush():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def step(k, fn):
    rec = {"step": k, "ok": False, "err": None, "tb": None, "data": None}
    try:
        rec["data"] = fn()
        rec["ok"] = True
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:400]
        rec["tb"] = traceback.format_exc().splitlines()[-1][:260]
    R["steps"].append(rec)
    flush()
    return rec


# 坑点：不要对已打开的包装再 cast()，两个包装会重复关闭同一对象，
# 导致后续 getLayerTable / 迭代 / 块表 抛 "Caught an unknown exception!"
# 正确做法：直接用 tr.openObject() 返回的包装（它已是具体类型）。


flush()

# ---------- B 绘图 ----------
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


def _pline():
    pl = P.Db.PyDbPolyline()
    pl.addVertexAt(0, P2(200, 0), 0.0, -1.0, -1.0)
    pl.addVertexAt(1, P2(240, 40), 1.0, -1.0, -1.0)
    pl.addVertexAt(2, P2(280, 0), 0.0, -1.0, -1.0)
    pl.setClosed(False)
    return pl


def _mtext():
    mt = P.Db.PyDbMText()
    mt.setLocation(P3(0, -40, 0))
    mt.setContents("PyArx 多行文字")
    mt.setWidth(80.0)
    mt.setHeight(5.0)
    return mt


def _spline():
    return P.Db.PyDbSpline([P3(300, 0, 0), P3(330, 30, 0), P3(360, 0, 0), P3(390, 30, 0)],
                           V3(1, 0, 0), V3(1, 0, 0), 0, 3)


def _solid3d():
    so = P.Db.PyDb3dSolid()
    so.createBox(40.0, 40.0, 40.0)
    m = P.Ge.AcGeMatrix3d()
    m.setToTranslation(V3(420, 0, 0))
    so.transformBy(m)
    return so


def b_draw():
    out = {}
    makers = [
        ("line", lambda: P.Db.PyDbLine(P3(0, 0, 0), P3(100, 0, 0))),
        ("circle", lambda: P.Db.PyDbCircle(P3(50, 50, 0), V3(0, 0, 1), 20)),
        ("arc", lambda: P.Db.PyDbArc(P3(0, 60, 0), 30, 0.0, math.pi)),
        ("text", lambda: P.Db.PyDbText(P3(0, -20, 0), "PyArx T8", 4.0, 0.0)),
        ("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50, 0), V3(0, 0, 1), V3(25, 0, 0), 0.4)),
        ("point", lambda: P.Db.PyDbPoint(P3(150, 0, 0))),
        ("solid", lambda: P.Db.PyDbSolid(P3(0, 100, 0), P3(20, 100, 0), P3(20, 120, 0), P3(0, 120, 0))),
        ("ray", _ray),
        ("xline", _xline),
        ("pline", _pline),
        ("mtext", _mtext),
        ("spline", _spline),
        ("solid3d", _solid3d),
    ]
    for k, maker in makers:
        try:
            with P.Db.PyDbTrans() as tr:
                e = maker()
                cls = str(e.className())
                ok = tr.addEntity(e)
                oid = e.objectId()
            IDS[k] = oid
            out[k] = {"add": ok, "id": oid, "class": cls}
        except Exception as ex:
            out[k] = {"err": type(ex).__name__ + ": " + str(ex)[:160]}
    out["count_in_db"] = len(db.objectIds("AcDbEntity"))
    return out


step("A_env", lambda: {"file": str(db.getFilename()), "entities": len(db.objectIds("AcDbEntity")),
                       "doc": str(doc.name)})
step("B_draw", b_draw)

# ---------- C 读回 ----------
def c_read():
    out = {}
    with P.Db.PyDbTrans() as tr:
        for k, oid in IDS.items():
            try:
                o = tr.openObject(oid, 0)
                if o is None:
                    out[k] = "None"
                    continue
                rec = {"wrapper": type(o).__name__, "class": str(o.className())}
                ext = o.getGeomExtents() if hasattr(o, "getGeomExtents") else None
                rec["ext"] = None if ext is None else [str(ext[0]), str(ext[1])]
                out[k] = rec
            except Exception as ex:
                out[k] = {"err": type(ex).__name__ + ": " + str(ex)[:120]}
    with P.Db.PyDbTrans() as tr:
        c = tr.openObject(IDS["circle"], 0)
        out["circle_radius"] = c.radius()
        l = tr.openObject(IDS["line"], 0)
        out["line_end"] = [l.endPoint().x, l.endPoint().y, l.endPoint().z]
        pl = tr.openObject(IDS["pline"], 0)
        out["pline_verts"] = pl.numVerts()
        out["pline_bulge"] = pl.getBulgeAt(1)
        t = tr.openObject(IDS["text"], 0)
        out["text"] = str(t.textString())
        out["text_h"] = t.height()
    return out


step("C_read", c_read)

# ---------- D 修改 ----------
def d_modify():
    out = {}
    with P.Db.PyDbTrans() as tr:
        l = tr.openObject(IDS["line"], 1))
        out["setColor"] = l.setColorIndex(3)
        out["color"] = l.colorIndex()
        out["setLinetype"] = l.setLinetype("CONTINUOUS")
        out["setLineWeight"] = l.setLineWeight(35)
    with P.Db.PyDbTrans() as tr:
        c = tr.openObject(IDS["circle"], 1))
        out["setRadius"] = c.setRadius(35.0)
        out["radius"] = c.radius()
    with P.Db.PyDbTrans() as tr:
        t = tr.openObject(IDS["text"], 1))
        out["setText"] = t.setTextString("PyArx T8 已修改")
        out["text_now"] = str(t.textString())
        out["setHeight"] = t.setHeight(6.0)
        out["height"] = t.height()
    with P.Db.PyDbTrans() as tr:
        pl = tr.openObject(IDS["pline"], 1))
        out["setPointAt"] = pl.setPointAt(0, P2(210, 10))
        out["setBulgeAt"] = pl.setBulgeAt(1, 0.5)
    with P.Db.PyDbTrans() as tr:
        a = tr.openObject(IDS["arc"], 1))
        m = P.Ge.AcGeMatrix3d()
        m.setToRotation(0.3, V3(0, 0, 1), P3(0, 0, 0))
        out["transform"] = a.transformBy(m)
        out["center"] = str(a.center())
    return out


step("D_modify", d_modify)

# ---------- E 图层 ----------
def e_layers():
    out = {}
    # 1) 建层（写）
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        out["setName"] = rec.setName("PYX_T8_LAYER")
        rec.setColor(200, 80, 40)
        out["add"] = lt.add(rec)
        IDS["layer"] = rec.objectId()
        lt.close()                      # 用完立即关闭，否则后续 getLayerTable 撞 eWasOpenForWrite
    out["layer_id"] = IDS["layer"]

    # 2) 枚举 + 按名取（读）
    with P.Db.PyDbTrans() as tr:
        lt2 = db.getLayerTable(0)
        out["has"] = lt2.has("PYX_T8_LAYER")
        out["keys_n"] = len(lt2.keys())
        out["values_n"] = len(lt2.values(0))
        out["items_n"] = len(lt2.items(0))
        out["toDict_n"] = len(lt2.toDict())
        out["numRecords"] = lt2.numRecords()
        r0 = lt2.getAt("PYX_T8_LAYER")
        out["getAt_name"] = str(r0.getName())
        out["getAt_color"] = str(r0.color())
        r0.close()
        lt2.close()

    # 3) 层状态开关（写）
    with P.Db.PyDbTrans() as tr:
        lt3 = db.getLayerTable()
        r = lt3.getAt("PYX_T8_LAYER")
        r.setIsOff(True)
        out["off"] = r.isOff()
        r.setIsOff(False)
        r.setIsLocked(True)
        out["locked"] = r.isLocked()
        r.setIsLocked(False)
        r.setIsFrozen(True)
        out["frozen"] = r.isFrozen()
        r.setIsFrozen(False)
        out["inUse"] = r.isInUse()
        r.close()
        lt3.close()

    # 4) 图元换层
    with P.Db.PyDbTrans() as tr:
        l = tr.openObject(IDS["line"], 1))
        out["setLayer"] = l.setLayer("PYX_T8_LAYER")
        out["layer_now"] = str(l.layer())
    return out


step("E_layers", e_layers)

# ---------- F 块表 ----------
def f_blocks():
    out = {}
    # 1) 定义块（写）
    with P.Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_T8_BLOCK")
        btr.appendAcDbEntity(P.Db.PyDbCircle(P3(0, 0, 0), V3(0, 0, 1), 10))
        btr.appendAcDbEntity(P.Db.PyDbLine(P3(-10, 0, 0), P3(10, 0, 0)))
        out["bt_add"] = bt.add(btr)
        IDS["blk"] = btr.objectId()
        out["blk"] = IDS["blk"]
        out["numEntities"] = btr.numEntities()
        out["entityIds"] = len(btr.entityIds())
        out["len"] = len(btr)
        bt.close()
    # 2) 块参照插入：三种写法分别验证（各自独立事务）
    variants = {}
    for name, fn in (("addEntity", _ref_add), ("append", _ref_append), ("refid", _ref_by_ctor)):
        try:
            variants[name] = fn()
        except Exception as ex:
            variants[name] = "ERR " + type(ex).__name__ + ": " + str(ex)[:160]
    out["refs"] = variants
    # 3) 回查块定义（读）
    with P.Db.PyDbTrans() as tr:
        bt2 = db.getBlockTable(0)
        br = bt2.getAt("PYX_T8_BLOCK", 0)
        out["block_name"] = str(br.getName())
        out["block_count"] = len(br.entityIds())
        br.close()
        bt2.close()
    out["space_len_after"] = _space_len()
    return out


def _ref_add():
    with P.Db.PyDbTrans() as tr:
        br = P.Db.PyDbBlockReference()
        br.setBlockTableRecord(IDS["blk"])
        br.setPosition(P3(500, 0, 0))
        ok = tr.addEntity(br)
        return {"add": ok, "id": br.objectId()}


def _ref_append():
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
        br = P.Db.PyDbBlockReference()
        br.setBlockTableRecord(IDS["blk"])
        br.setPosition(P3(560, 0, 0))
        ok = sp.appendAcDbEntity(br)
        return {"append": ok, "id": br.objectId()}


def _ref_by_ctor():
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 1)
        sp.appendAcDbEntityId(IDS["blk"])
        return {"appendId": True}


def _space_len():
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
        return sp.numEntities()


step("F_blocks", f_blocks)

# ---------- G 迭代协议（拆成三小项，定位到具体调用） ----------
def g_counts():
    out = {}
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
        for k, fn in (("num", lambda: sp.numEntities()),
                      ("len", lambda: len(sp)),
                      ("ids", lambda: len(sp.entityIds())),
                      ("values", lambda: len(sp.values(0))),
                      ("items", lambda: len(sp.items(0))),
                      ("keys", lambda: len(sp.keys()))):
            try:
                out[k] = fn()
            except Exception as ex:
                out[k] = "ERR " + type(ex).__name__ + ": " + str(ex)[:120]
        sp.close()
    return out


def g_iter_entities():
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
        n = 0
        cls = []
        for e in sp.iterEntities(0, True):
            n += 1
            cls.append(str(e.className()))
        sp.close()
    return {"n": n, "cls": cls[:4]}


def g_mapping():
    out = {}
    with P.Db.PyDbTrans() as tr:
        sp = P.Db.PyDbBlockTableRecord(db.currentSpaceId(), 0)
        ids = sp.entityIds()
        if ids:
            for k, fn in (("getitem_null", lambda: sp[ids[0]].isNull()),
                          ("get_none", lambda: sp.get(ids[0], None) is None),
                          ("contains", lambda: ids[0] in sp)):
                try:
                    out[k] = fn()
                except Exception as ex:
                    out[k] = "ERR " + type(ex).__name__ + ": " + str(ex)[:120]
        else:
            out["ids"] = 0
        sp.close()
    return out


step("G1_counts", g_counts)
step("G2_iter_entities", g_iter_entities)
step("G3_mapping", g_mapping)

# ---------- H Ed ----------
def h_ed():
    out = {}
    l = None
    with P.Db.PyDbTrans() as tr:
        l = tr.openObject(IDS["line"], 0))
        handle = str(l.handle())
    out["handle"] = handle
    ss = P.Ed.ssget("X", None, None, [(0, "LINE")])
    out["ss_n"] = None if ss is None else P.Ed.sslength(ss)
    if ss is not None and P.Ed.sslength(ss) > 0:
        en = P.Ed.ssname(ss, 0)
        dxf = P.Ed.entget(en)
        out["entget"] = {"0": dxf.get(0), "8": dxf.get(8), "62": dxf.get(62), "len": len(dxf)}
    out["handent"] = P.Ed.handent(handle) is not None
    out["entlast"] = P.Ed.entlast() is not None
    out["sssetfirst"] = P.Ed.sssetfirst([IDS["line"]])
    g = P.Ed.ssgetfirst()
    out["ssgetfirst"] = [len(g[0]), len(g[1])]
    out["sssetfirst_clear"] = P.Ed.sssetfirst([])
    return out


step("H_ed", h_ed)

# ---------- I 删除 ----------
def i_delete():
    out = {}
    n0 = len(db.objectIds("AcDbEntity"))
    with P.Db.PyDbTrans() as tr:
        e = tr.openObject(IDS["point"], 1)
        out["erase"] = e.erase()
        out["erased"] = e.isErased()
    n1 = len(db.objectIds("AcDbEntity"))
    with P.Db.PyDbTrans() as tr:
        e = tr.openObject(IDS["point"], 1, True)
        out["unerase"] = e.erase(False)
    n2 = len(db.objectIds("AcDbEntity"))
    out["count"] = {"before": n0, "erased": n1, "restored": n2}
    return out


step("I_delete", i_delete)

R["done"] = True
flush()
print("[PyArx] batch8 done")
