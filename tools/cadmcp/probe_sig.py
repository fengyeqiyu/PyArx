"""probe_sig.py —— 在 accoreconsole 无头环境里探测"高风险图元"的真实签名，
并把结论逐条落盘（即使进程崩溃，已写出的结论仍然可用）。

关注三类：Leader（曾因 setVertexAt 越界写崩 CAD）、Spline（需要真正的点数据）、
Hatch（需要 loop_type 与评估）。
"""

import json

OUT = r"G:\PyArx\tools\cadmcp\probe_out.json"
P = globals().get("P", None)

log = {"P_injected": P is not None}


def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


def rec(name, fn):
    try:
        log[name] = {"ok": True, "val": str(fn())[:400]}
    except Exception as e:
        log[name] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:600]}
    flush()


flush()

# ---- 0. 基本可用性
rec("curDb", lambda: P.Db.curDb())

# ---- 1. Leader：先探 appendVertex 签名，再真正建一条
rec("leader_append_sig", lambda: P.Db.PyDbLeader().appendVertex())


def leader_make():
    ld = P.Db.PyDbLeader()
    for x, y in [(0, 0), (100, 50), (200, 80)]:
        ld.appendVertex(P.Ge.AcGePoint3d(float(x), float(y), 0.0))
    ld.setToStraightLeader()
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(ld)
    return str(ld.handle())


rec("leader_make", leader_make)

# ---- 2. Spline：NURBS 构造函数签名 + 由拟合点建样条
rec("nurb_ctor_sig", lambda: P.Ge.AcGeNurbCurve3d("bad"))
rec("nurb_ctor_empty", lambda: P.Ge.AcGeNurbCurve3d())
rec("spline_setfit_sig", lambda: P.Db.PyDbSpline().setFitData())


def spline_make():
    sp = P.Db.PyDbSpline()
    pts = P.Ge.AcGePoint3dArray()
    for x, y in [(0, 0), (100, 55), (200, 5), (300, 85)]:
        pts.append(P.Ge.AcGePoint3d(float(x), float(y), 0.0))
    sp.setFitData(pts, 1e-10)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(sp)
    return str(sp.handle())


rec("spline_make", spline_make)

# ---- 3. Hatch：loop_type + 点数组 + 是否有评估接口
rec("hatch_apis", lambda: [a for a in dir(P.Db.PyDbHatch()) if "val" in a.lower() or "Loop" in a])


def hatch_make():
    h = P.Db.PyDbHatch()
    h.setDatabaseDefaults()
    h.setHatchObjectType(1)          # 1 = AcDbHatch::kPolyline?（探测用）
    h.setPattern("SOLID", 1)         # 探测签名
    pts = [P.Ge.AcGePoint2d(0.0, 0.0), P.Ge.AcGePoint2d(200.0, 0.0),
           P.Ge.AcGePoint2d(200.0, 150.0), P.Ge.AcGePoint2d(0.0, 150.0)]
    ok = h.appendLoopPoints(0, pts, [])
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(h)
    return {"append": ok, "handle": str(h.handle())}


rec("hatch_make", hatch_make)

flush()
log["done"] = True
flush()
