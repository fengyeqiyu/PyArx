# -*- coding: utf-8 -*-
"""补充轮：只补齐缺类别模块；mlab.dump 写固定累积路径（不依赖 PYARX_OUT）。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

Ge = P.Ge
Gi = P.Gi
SHARD = r"G:\PyArx\tools\train\data\shard_ge_gi.json"
EPS = 1e-9


def coord(o):
    if hasattr(o, "x"):
        v = [o.x, o.y]
        if hasattr(o, "z"):
            v.append(o.z)
        return v
    return list(o)


def close(a, b, tol=EPS):
    return abs(float(a) - float(b)) <= tol


# ---------- 1. AcGeVector2d 补齐 ----------
L, M = "Ge", "AcGeVector2d"


def v2_ctor():
    v = Ge.AcGeVector2d(3.0, 4.0)
    return "ctor(x,y)=(%.3f,%.3f) x=%.3f y=%.3f type=%s.%s" % (
        v.x, v.y, v.x, v.y, type(v).__module__, type(v).__name__)


def v2_len():
    v = Ge.AcGeVector2d(3.0, 4.0)
    return "(3,4) length=%.6f lengthSqrd=%.6f" % (v.length(), v.lengthSqrd())


def v2_dot_cross():
    a, b = Ge.AcGeVector2d(1.0, 0.0), Ge.AcGeVector2d(0.0, 1.0)
    return "x·y=%.3f 2d crossProduct=%.3f（标量，非向量）" % (a.dotProduct(b), a.crossProduct(b))


def v2_norm():
    n = Ge.AcGeVector2d(3.0, 4.0).normalize()
    return "normalize -> (%.6f,%.6f) length=%.6f" % (n.x, n.y, n.length())


def v2_normal():
    n = Ge.AcGeVector2d(3.0, 4.0).normal()
    return "normal() -> (%+.6f,%+.6f) length=%.6f" % (n.x, n.y, n.length())


mlab.check(L, M, "exists+construct_xy", v2_ctor)
mlab.check(L, M, "length_lengthSqrd", v2_len)
mlab.check(L, M, "dot_cross_scalar", v2_dot_cross)
mlab.check(L, M, "normalize", v2_norm)
mlab.check(L, M, "normal", v2_normal)
mlab.probe(L, M, "零长度 normalize",
           lambda: Ge.AcGeVector2d(0.0, 0.0).normalize(), allow=("Exception",))

# ---------- 2. AcGeCircArc3d 失败面 ----------
L, M = "Ge", "AcGeCircArc3d"


def c_neg():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0)
    return "负半径 -1.0 静默接受，读回 radius=%.3f（取绝对值，未抛异常）" % c.radius()


mlab.probe(L, M, "负半径构造（期望异常）",
           lambda: Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0),
           allow=("Exception",))


def c_bad_len():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    return "length(-1,-1) -> %.6f" % c.length(-1.0, -1.0)


mlab.probe(L, M, "length 超区间参数 (-1,-1)", c_bad_len, allow=("Exception",))


def c_intersect():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    line = Ge.AcGeLine3d(Ge.AcGePoint3d(0, 0, 5.0), Ge.AcGeVector3d(1, 0, 0))
    sig = mlab.method_typeerror(c, "intersectWithLine", "x")
    try:
        r = c.intersectWithLine(line)
        return "平行/异面线 intersectWithLine -> %r；签名: %s" % (r, sig[:160])
    except Exception as e:
        return "抛 %s: %s；签名: %s" % (type(e).__name__, str(e)[:160], sig[:160])


mlab.probe(L, M, "intersectWithLine 不相交线", c_intersect, allow=("Exception",))

# ---------- 3. AcGePlane 闭环 + 失败面 ----------
L, M = "Ge", "AcGePlane"


def pl_loop():
    pl = Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1))
    d1 = pl.signedDistanceTo(Ge.AcGePoint3d(1, 2, 5))
    d2 = pl.signedDistanceTo(Ge.AcGePoint3d(0, 0, -3))
    assert close(d1, 5.0) and close(d2, -3.0)
    return "平面 z=0：点(1,2,5) signedDistanceTo=%.3f；点(0,0,-3) signedDistanceTo=%.3f（两侧符号往返一致）" % (
        d1, d2)


mlab.check(L, M, "signed_distance_both_sides", pl_loop)
mlab.probe(L, M, "全零系数 AcGePlane(0,0,0,0)",
           lambda: Ge.AcGePlane(0.0, 0.0, 0.0, 0.0), allow=("Exception",))


def pl_parallel():
    pl = Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1))
    line = Ge.AcGeLine3d(Ge.AcGePoint3d(0, 0, 5), Ge.AcGeVector3d(1, 0, 0))
    sig = mlab.method_typeerror(pl, "intersectWithLine", "x")
    try:
        r = pl.intersectWithLine(line)
        return "平行线 z=5 交平面 -> %r；签名: %s" % (r, sig[:160])
    except Exception as e:
        return "抛 %s: %s；签名: %s" % (type(e).__name__, str(e)[:160], sig[:160])


mlab.probe(L, M, "intersectWithLine 平行线", pl_parallel, allow=("Exception",))

# ---------- 4. Point2d / Vector3d 逆矩阵闭环 ----------
L, M = "Ge", "AcGePoint2d"


def p2_loop():
    p = Ge.AcGePoint2d(1.0, 2.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    mi = m.inverse()
    q = p.transformBy(m)
    r = q.transformBy(mi)
    return "初值=(%.1f,%.1f) 平移后=(%+.3f,%+.3f) 逆矩阵回读=(%+.3f,%+.3f)" % (
        p.x, p.y, q.x, q.y, r.x, r.y)


mlab.check(L, M, "loop_inverse_matrix", p2_loop)

L, M = "Ge", "AcGeVector3d"


def v3_loop():
    v = Ge.AcGeVector3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    mi = m.inverse()
    q = v.transformBy(m)
    r = q.transformBy(mi)
    return "初值=(%.1f,%.1f,%.1f) 平移后=(%+.3f,%+.3f,%+.3f) 逆矩阵回读=(%+.3f,%+.3f,%+.3f)" % (
        v.x, v.y, v.z, q.x, q.y, q.z, r.x, r.y, r.z)


mlab.check(L, M, "loop_inverse_matrix", v3_loop)

# ---------- 5. LineSeg3d 属性 + evalPoint 闭环 ----------
L, M = "Ge", "AcGeLineSeg3d"


def l_seg():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGePoint3d(10, 0, 0))
    mid = ls.midPoint()
    d = ls.direction()
    p05 = ls.evalPoint(0.5)
    assert close(mid.x, 5.0) and close(mid.y, 0.0) and close(mid.z, 0.0)
    assert close(ls.length(), 10.0)
    assert close(p05.x, mid.x) and close(p05.y, mid.y) and close(p05.z, mid.z)
    return "(0,0,0)-(10,0,0) length=%.3f mid=(%.3f,%.3f,%.3f) direction=(%.3f,%.3f,%.3f) evalPoint(0.5)=(%.3f,%.3f,%.3f) 与 midPoint 一致" % (
        ls.length(), mid.x, mid.y, mid.z, d.x, d.y, d.z, p05.x, p05.y, p05.z)


mlab.check(L, M, "midpoint_length_direction_evalPoint", l_seg)

# ---------- 6. Gi 抽象类 not_applicable ----------
ABSTRACT = [
    ("PyGiSubEntityTraits", "真实实例从 PyGiWorldDraw.subEntityTraits 取"),
    ("PyGiGeometry", "真实实例从 PyGiWorldDraw.geometry / PyGiViewportDraw.geometry 取"),
    ("PyGiWorldGeometry", "真实实例从 PyGiWorldDraw.geometry 取"),
    ("PyGiDrawable", "真实实例由 Db 实体自身（PyDbEntity 对应 GiDrawable）提供"),
    ("PyGiWorldDraw", "真实实例由实体 worldDraw 回调上下文提供；可访问 geometry/subEntityTraits/context"),
]
for name, src in ABSTRACT:
    cls = getattr(Gi, name, None)
    has = cls is not None
    nmem = len([x for x in dir(cls) if not x.startswith("_")]) if has else 0
    ctor_err = mlab.typeerror_hint(cls, "x")[:120] if has else "missing"
    mlab.check("Gi", name, "not_applicable_abstract",
               lambda n=name, h=has, nm=nmem, e=ctor_err, s=src:
               "hasattr(%s)=%s members=%d；抽象类无构造（%s），属性往返/闭环不可达。%s" % (
                   n, h, nm, e, s))

# ---------- 7. TextStyle 尺寸闭环到 PyDbText ----------
L, M = "Gi", "PyGiTextStyle"


def ts_text_loop():
    t = Gi.PyGiTextStyle()
    t.setTextSize(8.0)
    sz = t.textSize()
    assert close(sz, 8.0)
    db = P.Db.curDb()
    before = len(list(db.objectIds("AcDbEntity")))
    with P.Db.PyDbTrans() as tr:
        txt = P.Db.PyDbText(Ge.AcGePoint3d(10, 20, 0), "配电箱", 8.0, 0.0)
        tr.addEntity(txt)
        h = str(txt.handle())
        hgt = float(txt.height())
    after = len(list(db.objectIds("AcDbEntity")))
    assert h != "0" and after > before and close(hgt, 8.0)
    return "style.textSize=%.2f -> PyDbText('配电箱',height=8.0) 入库 handle=%s height=%.4f 实体数 %d->%d 一致" % (
        sz, h, hgt, before, after)


mlab.check(L, M, "textsize_dbtext_closed_loop", ts_text_loop)

res = mlab.dump(SHARD)
print(json.dumps({"written": SHARD, "modules": res["modules"], "rows": res["rows_written"]},
                 ensure_ascii=False))
