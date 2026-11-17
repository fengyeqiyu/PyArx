# -*- coding: utf-8 -*-
"""batch14 全功能：图元无事务写入 + 符号表最小事务(try/finally 防泄漏)。
不保存、不改名、不关闭图形。所有过程写入 pyarx_learn.log。
"""
import datetime, json, traceback

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_test_batch14.json"
IDS = {}
R = {"items": [], "done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def flush():
    with open(RES, "w", encoding="utf-8") as f:
        json.dump(R, f, ensure_ascii=False, indent=1, default=str)


def item(name, desc, fn):
    rec = {"item": name, "desc": desc, "ok": False}
    log("START", "%s | %s" % (name, desc))
    try:
        rec["data"] = fn()
        rec["ok"] = True
        log("OK", name + " " + json.dumps(rec["data"], ensure_ascii=False, default=str)[:320])
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:220]
        log("FAIL", name + " " + rec["err"])
        log("TRACE", traceback.format_exc().splitlines()[-1][:200])
    R["items"].append(rec)
    flush()


def P2(x, y):
    return P.Ge.AcGePoint2d(float(x), float(y))


def P3(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def V3(x, y, z=0.0):
    return P.Ge.AcGeVector3d(float(x), float(y), float(z))


def with_table(kind, fn):
    """最小事务操作符号表：成功提交，失败 abort，无论成败都 close 表包装。"""
    tr = P.Db.PyDbTrans()
    tbl = None
    try:
        tbl = DB.getBlockTable(1) if kind == "block" else DB.getLayerTable(1)
        out = fn(tbl)
        tr.commit()
        return out
    except Exception:
        try:
            tr.abort()
        except Exception:
            pass
        raise
    finally:
        if tbl is not None:
            try:
                tbl.close()
            except Exception:
                pass
        tbl = None


DB = P.Db.curDb()
SPACE = DB.currentSpaceId()
LAYER = "PYX_T14_LAYER"
BLOCK = "PYX_T14_BLOCK"
log("====", "batch14 开始 entities=%s" % len(DB.objectIds("AcDbEntity")))
flush()


def t1_draw():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    makers = [
        ("line", lambda: P.Db.PyDbLine(P3(0, 0), P3(100, 0))),
        ("circle", lambda: P.Db.PyDbCircle(P3(50, 50), V3(0, 0, 1), 20)),
        ("arc", lambda: P.Db.PyDbArc(P3(0, 60), 30, 0.0, 3.14159)),
        ("text", lambda: P.Db.PyDbText(P3(0, -20), "PyArx T14", 4.0, 0.0)),
        ("ellipse", lambda: P.Db.PyDbEllipse(P3(120, 50), V3(0, 0, 1), V3(25, 0, 0), 0.4)),
        ("point", lambda: P.Db.PyDbPoint(P3(150, 0))),
        ("solid", lambda: P.Db.PyDbSolid(P3(0, 100), P3(20, 100), P3(20, 120), P3(0, 120))),
        ("pline", lambda: _pline()),
    ]
    for k, mk in makers:
        IDS[k] = sp.appendAcDbEntityId(mk())
        out[k] = IDS[k]
    out["num"] = sp.numEntities()
    sp.close()
    out["db"] = len(DB.objectIds("AcDbEntity"))
    return out


def _pline():
    pl = P.Db.PyDbPolyline()
    pl.addVertexAt(0, P2(200, 0), 0.0, -1.0, -1.0)
    pl.addVertexAt(1, P2(240, 40), 0.5, -1.0, -1.0)
    pl.addVertexAt(2, P2(280, 0), 0.0, -1.0, -1.0)
    return pl


def t2_layer():
    out = {}

    def work(lt):
        rec = P.Db.PyDbLayerTableRecord.newRecord()
        rec.setName(LAYER)
        rec.setColor(200, 80, 40)
        out["add"] = lt.add(rec)
        out["id"] = rec.objectId()
        return out

    with_table("layer", work)
    lt = DB.getLayerTable(0)
    out["has"] = lt.has(LAYER)
    lt.close()
    return out


def t3_block():
    out = {}

    def work(bt):
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName(BLOCK)
        out["child1"] = btr.appendAcDbEntity(P.Db.PyDbCircle(P3(0, 0), V3(0, 0, 1), 10))
        out["child2"] = btr.appendAcDbEntity(P.Db.PyDbLine(P3(-10, 0), P3(10, 0)))
        out["bt_add"] = bt.add(btr)
        IDS["blk"] = btr.objectId()
        out["blk"] = IDS["blk"]
        out["block_ents"] = btr.numEntities()
        return out

    with_table("block", work)
    return out


def t4_insert():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    br = P.Db.PyDbBlockReference()
    out["setBtr"] = br.setBlockTableRecord(IDS["blk"])
    out["setPos"] = br.setPosition(P3(500, 0))
    out["ref"] = sp.appendAcDbEntityId(br)
    out["space_num"] = sp.numEntities()
    sp.close()
    return out


def _first_id(cls):
    ids = DB.objectIds(cls)
    return ids[0] if ids else None


def t5_read():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
    out["num"] = sp.numEntities()
    for cls in ("AcDbLine", "AcDbCircle", "AcDbArc", "AcDbText", "AcDbEllipse",
                "AcDbPolyline", "AcDbPoint", "AcDbSolid", "AcDbBlockReference"):
        out[cls] = len(DB.objectIds(cls))
    oid = _first_id("AcDbCircle")
    if oid is not None:
        e = sp.getEntityById(oid, 0)
        c = P.Db.PyDbCircle.cast(e)
        out["circle"] = {
            "r": c.radius(),
            "center": [c.center().x, c.center().y],
            "color": c.colorIndex(),
            "layer": str(c.layer()),
            "ext": [str(x) for x in c.getGeomExtents()],
        }
        e.close()
    oid = _first_id("AcDbLine")
    if oid is not None:
        e = sp.getEntityById(oid, 0)
        ln = P.Db.PyDbLine.cast(e)
        out["line"] = {"start": [ln.startPoint().x, ln.startPoint().y],
                       "end": [ln.endPoint().x, ln.endPoint().y]}
        e.close()
    oid = _first_id("AcDbPolyline")
    if oid is not None:
        e = sp.getEntityById(oid, 0)
        pl = P.Db.PyDbPolyline.cast(e)
        out["pline"] = {"verts": pl.numVerts(), "closed": pl.isClosed()}
        e.close()
    sp.close()
    log("坑点", "PyDbXxx.cast 不做类型检查，必须先用 db.objectIds('AcDbCircle') 按类过滤再 cast，否则调用错方法必 unknown exception")
    return out


def t6_modify():
    out = {}
    oid = _first_id("AcDbCircle")
    if oid is None:
        return {"skip": "no circle"}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    e = sp.getEntityById(oid, 1)
    c = P.Db.PyDbCircle.cast(e)
    out["setColor"] = c.setColorIndex(3)
    out["setLayer"] = c.setLayer(LAYER)
    out["setRadius"] = c.setRadius(35.0)
    out["radius"] = c.radius()
    out["color"] = c.colorIndex()
    e.close()
    sp.close()
    return out


def t7_iter():
    out = {}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 0)
    out["num"] = sp.numEntities()
    out["len"] = len(sp)
    out["ids"] = len(sp.entityIds())
    n = 0
    for e in sp.iterEntities(0, True):
        n += 1
        e.close()
    out["iter"] = n
    sp.close()
    return out


def t8_ed():
    out = {}
    ss = P.Ed.ssget("X", None, None, [(0, "LINE")])
    out["ss_n"] = None if ss is None else P.Ed.sslength(ss)
    if ss is not None and P.Ed.sslength(ss) > 0:
        dxf = P.Ed.entget(P.Ed.ssname(ss, 0))
        out["entget"] = {"0": dxf.get(0), "8": dxf.get(8), "62": dxf.get(62)}
    out["entlast"] = P.Ed.entlast() is not None
    out["select"] = P.Ed.sssetfirst([IDS["line"]])
    g = P.Ed.ssgetfirst()
    out["ssgetfirst"] = [len(g[0]), len(g[1])]
    out["clear"] = P.Ed.sssetfirst([])
    return out


def t9_delete():
    out = {}
    n0 = len(DB.objectIds("AcDbEntity"))
    oid = _first_id("AcDbPoint")
    if oid is None:
        return {"skip": "no point"}
    sp = P.Db.PyDbBlockTableRecord(SPACE, 1)
    e = sp.getEntityById(oid, 1)
    out["isNull"] = e.isNull()
    out["erase"] = e.erase()
    out["erased"] = e.isErased()
    n1 = len(DB.objectIds("AcDbEntity"))
    out["unerase"] = e.erase(False)
    out["erased_after"] = e.isErased()
    e.close()
    sp.close()
    out["count"] = {"before": n0, "erased": n1, "after": len(DB.objectIds("AcDbEntity"))}
    return out


item("T1_draw", "创建 8 类图元（无事务 appendAcDbEntityId）", t1_draw)
item("T2_layer", "建图层并确认 has()", t2_layer)
item("T3_block", "建块定义（圆+直线）", t3_block)
item("T4_insert", "插入块参照到模型空间", t4_insert)
item("T5_read", "读回实体类型/包围盒/关键几何", t5_read)
item("T6_modify", "改圆：颜色/图层/半径", t6_modify)
item("T7_iter", "空间计数与 iterEntities 遍历", t7_iter)
item("T8_ed", "Ed：ssget/entget/sssetfirst", t8_ed)
item("T9_delete", "删除 POINT 并恢复", t9_delete)

R["done"] = True
flush()
log("====", "batch14 结束")
print("[PyArx] batch14 done")
