"""headless_entity_test.py —— 在 accoreconsole（无头）里验证"危险的"图元创建：
Leader / Spline / Hatch / Table / 3D 多段线 / 块定义+块参照。

逐项执行、每步把结果 flush 到 JSON，进程就算崩了也知道死在哪一项。
"""

import json

OUT = r"G:\PyArx\tools\cadmcp\headless_out.json"

P = globals().get("P")
if P is None:
    import PyArxCore as P

log = {}


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


def rec(name, fn):
    try:
        v = fn()
        log[name] = {"ok": True, "val": str(v)[:300]}
    except Exception as e:
        log[name] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:500]}
    flush()


def _pt(x, y, z=0.0):
    return P.Ge.AcGePoint3d(float(x), float(y), float(z))


def _add(e):
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(e)
    return str(e.handle())


def count_entities():
    db = P.Db.curDb()
    n = 0
    with P.Db.PyDbTrans() as tr:
        for _ in db.objectIds("AcDbEntity"):
            n += 1
    return n


flush()
rec("entity_count_before", count_entities)


# 1. Leader：appendVertex（替代曾经越界的 setVertexAt）
def leader_make():
    ld = P.Db.PyDbLeader()
    for x, y in [(0, 0), (60, 40), (120, 70)]:
        ok = ld.appendVertex(_pt(x, y))
    ld.setToStraightLeader()
    return _add(ld) + " append_ok=" + str(ok)


rec("leader_make", leader_make)


# 2. Spline：setFitData(points, startTan, endTan, knotParam, degree, tol)
def spline_make():
    sp = P.Db.PyDbSpline()
    pts = [_pt(0, 0), _pt(80, 50), _pt(160, 0), _pt(240, 70)]
    ok = sp.setFitData(pts, P.Ge.AcGeVector3d(1, 0, 0), P.Ge.AcGeVector3d(1, 0, 0), 0, 3, 0.0)
    return _add(sp) + " setFitData=" + str(ok)


rec("spline_make", spline_make)


# 3. Hatch：appendLoopPoints(loop_type, points, bulges) + evaluateHatch
def hatch_make():
    h = P.Db.PyDbHatch()
    h.setDatabaseDefaults()
    ok1 = h.setPattern(1, "SOLID")
    pts = [P.Ge.AcGePoint2d(0.0, 0.0), P.Ge.AcGePoint2d(150.0, 0.0),
           P.Ge.AcGePoint2d(150.0, 100.0), P.Ge.AcGePoint2d(0.0, 100.0)]
    ok2 = h.appendLoopPoints(1, pts, [])
    ok3 = h.evaluateHatch()
    hnd = _add(h)
    return "h=" + hnd + " pattern=" + str(ok1) + " loop=" + str(ok2) + " eval=" + str(ok3)


rec("hatch_make", hatch_make)


# 4. Table
def table_make():
    t = P.Db.PyDbTable()
    t.setSize(3, 2)
    t.setPosition(_pt(300, 0))
    for c in range(2):
        t.setColumnWidth(c, 40.0)
    for r in range(3):
        t.setRowHeight(r, 8.0)
        for c in range(2):
            t.setTextString(r, c, "R%dC%d" % (r, c))
    return _add(t)


rec("table_make", table_make)


# 5. 3D 多段线
def poly3d_make():
    pl = P.Db.PyDbPolyline3d()
    for pt in [(0, 0, 0), (100, 0, 50), (200, 100, 80)]:
        pl.appendVertex(P.Db.PyDb3dPolylineVertex(_pt(*pt)))
    pl.setClosed(False)
    return _add(pl)


rec("poly3d_make", poly3d_make)


# 6. 块定义 + 块参照
def block_make():
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        bt = tr.openObject(db.blockTableId(), 1)
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYARX_BLK")
        bt.add(btr)
        tr.addNewlyCreatedDBObject(btr, True)
        c = P.Db.PyDbCircle(_pt(0, 0), P.Ge.AcGeVector3d(0, 0, 1), 20.0)
        btr.appendAcDbEntity(c)
        tr.addNewlyCreatedDBObject(c, True)
    with P.Db.PyDbTrans() as tr:
        bt = tr.openObject(db.blockTableId(), 0)
        rec2 = bt.getAt("PYARX_BLK", 0)
        br = P.Db.PyDbBlockReference()
        br.setBlockTableRecord(rec2.objectId())
        br.setPosition(_pt(600, 0))
        tr.addEntity(br)
    return str(br.handle())


rec("block_make", block_make)


# 7. 图层
def layer_make():
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        lt = tr.openObject(db.layerTableId(), 1)
        lr = P.Db.PyDbLayerTableRecord.newRecord()
        lr.setName("PYARX_LAYER")
        lt.add(lr)
        tr.addNewlyCreatedDBObject(lr, True)
    return "PYARX_LAYER"


rec("layer_make", layer_make)

rec("entity_count_after", count_entities)
log["done"] = True
flush()
