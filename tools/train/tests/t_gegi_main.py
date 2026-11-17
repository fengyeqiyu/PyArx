# -*- coding: utf-8 -*-
"""Ge/Gi 无头实测主脚本：四类断言落 matrix，结果由 mlab.dump 写到 PYARX_OUT。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

Ge = P.Ge
Gi = P.Gi
EPS = 1e-9


def coord(o, idx):
    if hasattr(o, "x"):
        v = [o.x, o.y]
        if hasattr(o, "z"):
            v.append(o.z)
        return v
    return list(o)


def close(a, b, tol=EPS):
    return abs(float(a) - float(b)) <= tol


# ============================== Ge.AcGePoint3d ==============================
L, M = "Ge", "AcGePoint3d"


def g3_ctor():
    assert hasattr(Ge, "AcGePoint3d")
    p = Ge.AcGePoint3d(1.5, -2.5, 3.5)
    assert close(p.x, 1.5) and close(p.y, -2.5) and close(p.z, 3.5)
    z = Ge.AcGePoint3d()
    assert close(z.x, 0.0) and close(z.y, 0.0) and close(z.z, 0.0)
    c = Ge.AcGePoint3d(p)
    return "ctor(x,y,z)=(%.3f,%.3f,%.3f) 无参=(%.1f,%.1f,%.1f) 拷贝=(%.3f,%.3f,%.3f) type=%s.%s" % (
        p.x, p.y, p.z, z.x, z.y, z.z, c.x, c.y, c.z, type(p).__module__, type(p).__name__)


def g3_const():
    o = Ge.AcGePoint3d.kOrigin
    v = coord(o)
    assert all(close(x, 0.0) for x in v), "kOrigin=%r" % (v,)
    return "kOrigin=%r" % (v,)


def g3_distance():
    a = Ge.AcGePoint3d(0.0, 0.0, 0.0)
    b = Ge.AcGePoint3d(3.0, 4.0, 0.0)
    d = a.distanceTo(b)
    assert close(d, 5.0), "distanceTo=%.6f" % d
    return "(0,0,0) distanceTo (3,4,0) = %.6f (期望 5.0)" % d


def g3_transform_translate():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    q = p.transformBy(m)
    v = coord(q)
    assert close(v[0], 11.0) and close(v[1], 2.0) and close(v[2], 3.0), "q=%r" % (v,)
    return "(1,2,3) transformBy translate(10,0,0) -> (%+.3f,%+.3f,%+.3f)" % (v[0], v[1], v[2])


def g3_loop_inv():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(7.0, -4.0, 2.0))
    mi = m.inverse()
    q = mi.transformBy(m.transformBy(p))
    v = coord(q)
    assert close(v[0], 1.0) and close(v[1], 2.0) and close(v[2], 3.0), "roundtrip=%r" % (v,)
    return "translate(7,-4,2) -> inverse() 反变换 -> (%+.3f,%+.3f,%+.3f) 回到初值" % (v[0], v[1], v[2])


mlab.check(L, M, "exists+construct", g3_ctor)
mlab.check(L, M, "class_constant", g3_const)
mlab.check(L, M, "distanceTo", g3_distance)
mlab.check(L, M, "transformBy_translation", g3_transform_translate)
mlab.check(L, M, "loop_inverse_matrix", g3_loop_inv)
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGePoint3d((1.0, 2.0, 3.0)), allow=("Exception",))
mlab.probe(L, M, "distanceTo 传元组",
           lambda: Ge.AcGePoint3d().distanceTo((1.0, 2.0, 3.0)), allow=("Exception",))

# ============================== Ge.AcGePoint2d ==============================
L, M = "Ge", "AcGePoint2d"


def g2_ctor():
    p = Ge.AcGePoint2d(2.5, -1.5)
    assert close(p.x, 2.5) and close(p.y, -1.5)
    z = Ge.AcGePoint2d()
    assert close(z.x, 0.0) and close(z.y, 0.0)
    c = Ge.AcGePoint2d(p)
    return "ctor(x,y)=(%.3f,%.3f) 无参=0 拷贝=(%.3f,%.3f) type=%s.%s" % (
        p.x, p.y, c.x, c.y, type(p).__module__, type(p).__name__)


def g2_const():
    o = Ge.AcGePoint2d.kOrigin
    return "kOrigin=%r" % (coord(o),)


def g2_dist():
    a = Ge.AcGePoint2d(0.0, 0.0)
    b = Ge.AcGePoint2d(3.0, 4.0)
    d = a.distanceTo(b)
    assert close(d, 5.0), "%.6f" % d
    return "(0,0) distanceTo (3,4) = %.6f (期望 5.0)" % d


def g2_asvec():
    p = Ge.AcGePoint2d(2.0, 3.0)
    v = p.asVector()
    return "asVector -> (%+.3f,%+.3f)" % (v.x, v.y)


mlab.check(L, M, "exists+construct", g2_ctor)
mlab.check(L, M, "class_constant", g2_const)
mlab.check(L, M, "distanceTo", g2_dist)
mlab.check(L, M, "asVector", g2_asvec)
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGePoint2d((1.0, 2.0)), allow=("Exception",))

# ============================== Ge.AcGeVector3d ==============================
L, M = "Ge", "AcGeVector3d"


def v3_ctor():
    v = Ge.AcGeVector3d(3.0, 4.0, 0.0)
    assert close(v.x, 3.0) and close(v.y, 4.0) and close(v.z, 0.0)
    return "ctor(x,y,z)=(%.3f,%.3f,%.3f) type=%s.%s" % (
        v.x, v.y, v.z, type(v).__module__, type(v).__name__)


def v3_const():
    x, y, z = Ge.AcGeVector3d.kXAxis, Ge.AcGeVector3d.kYAxis, Ge.AcGeVector3d.kZAxis
    return "kXAxis=%r kYAxis=%r kZAxis=%r" % (coord(x), coord(y), coord(z))


def v3_length():
    v = Ge.AcGeVector3d(3.0, 4.0, 0.0)
    L2 = v.length()
    Lsq = v.lengthSqrd()
    assert close(L2, 5.0) and close(Lsq, 25.0)
    return "v(3,4,0).length=%.6f lengthSqrd=%.6f" % (L2, Lsq)


def v3_dot_cross():
    a = Ge.AcGeVector3d(1.0, 0.0, 0.0)
    b = Ge.AcGeVector3d(0.0, 1.0, 0.0)
    d = a.dotProduct(b)
    c = a.crossProduct(b)
    assert close(d, 0.0)
    assert close(c.z, 1.0) and close(c.x, 0.0) and close(c.y, 0.0)
    return "x·y=%.3f  x×y=(%.3f,%.3f,%.3f)" % (d, c.x, c.y, c.z)


def v3_normalize():
    v = Ge.AcGeVector3d(3.0, 4.0, 0.0)
    n = v.normalize()
    assert close(n.length(), 1.0)
    return "(3,4,0) normalize -> (%.6f,%.6f,%.6f) length=%.6f" % (n.x, n.y, n.z, n.length())


def v3_neg():
    v = Ge.AcGeVector3d(1.0, -2.0, 3.0)
    n = v.negate()
    return "(1,-2,3) negate -> (%+.3f,%+.3f,%+.3f) isZeroLength(v)=%s" % (
        n.x, n.y, n.z, v.isZeroLength())


mlab.check(L, M, "exists+construct", v3_ctor)
mlab.check(L, M, "class_constant", v3_const)
mlab.check(L, M, "length_lengthSqrd", v3_length)
mlab.check(L, M, "dotProduct_crossProduct", v3_dot_cross)
mlab.check(L, M, "normalize", v3_normalize)
mlab.check(L, M, "negate", v3_neg)
mlab.probe(L, M, "零向量 normalize", lambda: Ge.AcGeVector3d(0.0, 0.0, 0.0).normalize(),
           allow=("Exception",))
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGeVector3d((1.0, 2.0, 3.0)), allow=("Exception",))

# ============================== Ge.AcGeVector2d ==============================
L, M = "Ge", "AcGeVector2d"


def v2_ctor():
    v = Ge.AcGeVector2d(3.0, 4.0)
    return "ctor(x,y)=(%.3f,%.3f) length=%.6f type=%s.%s" % (
        v.x, v.y, v.length(), type(v).__module__, type(v).__name__)


def v2_dot_cross():
    a = Ge.AcGeVector2d(1.0, 0.0)
    b = Ge.AcGeVector2d(0.0, 1.0)
    return "x·y=%.3f cross=%.3f" % (a.dotProduct(b), a.crossProduct(b))


mlab.check(L, M, "exists+construct", v2_ctor)
mlab.check(L, M, "dot_cross", v2_dot_cross)
mlab.probe(L, M, "零向量 normalize", lambda: Ge.AcGeVector2d(0.0, 0.0).normalize(), allow=("Exception",))

# ============================== Ge.AcGeMatrix3d ==============================
L, M = "Ge", "AcGeMatrix3d"


def m_ctor():
    m = Ge.AcGeMatrix3d()
    det = m.det()
    idt = m.isSingular()
    return "无参构造 det=%.6f isSingular=%s type=%s.%s" % (
        det, idt, type(m).__module__, type(m).__name__)


def m_translate():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    t = m.translation()
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    q = p.transformBy(m)
    return "setToTranslation(10,0,0) translation=%r  (1,2,3)->(%+.3f,%+.3f,%+.3f)" % (
        coord(t), q.x, q.y, q.z)


def m_scale():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(2.0, Ge.AcGeVector3d(0.0, 0.0, 0.0))
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    q = p.transformBy(m)
    return "setToScaling(2,origin) (1,2,3)->(%+.3f,%+.3f,%+.3f)" % (q.x, q.y, q.z)


def m_loop():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(5.0, 5.0, 5.0))
    mi = m.inverse()
    q = mi.transformBy(m.transformBy(p))
    assert close(q.x, 1.0) and close(q.y, 2.0) and close(q.z, 3.0)
    return "平移矩阵 inverse() 闭环回到 (%+.3f,%+.3f,%+.3f)" % (q.x, q.y, q.z)


mlab.check(L, M, "exists+construct", m_ctor)
mlab.check(L, M, "setToTranslation", m_translate)
mlab.check(L, M, "setToScaling", m_scale)
mlab.check(L, M, "loop_inverse", m_loop)
mlab.probe(L, M, "逆矩阵形式提示",
           lambda: mlab.method_typeerror(Ge.AcGeMatrix3d(), "inverse", "x"), allow=("Exception",))


def singular_probe():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(0.0, Ge.AcGeVector3d(0.0, 0.0, 0.0))
    info = {"isSingular": m.isSingular(), "det": m.det()}
    try:
        mi = m.inverse()
        info["inverse"] = "returned matrix"
    except Exception as e:
        info["inverse"] = "%s: %s" % (type(e).__name__, str(e)[:160])
    return info


mlab.probe(L, M, "奇异矩阵 inverse", singular_probe, allow=("Exception",))

# ============================== Ge.AcGeScale3d ==============================
L, M = "Ge", "AcGeScale3d"


def s_ctor():
    s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
    f = Ge.AcGeScale3d(1.5)
    return "ctor(sx,sy,sz)=(%.3f,%.3f,%.3f) ctor(factor)=(%.3f,%.3f,%.3f) type=%s.%s" % (
        s.sx, s.sy, s.sz, f.sx, f.sy, f.sz, type(s).__module__, type(s).__name__)


def s_matrix():
    s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
    m = s.getMatrix()
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    q = p.transformBy(m)
    assert close(q.x, 2.0) and close(q.y, 6.0) and close(q.z, 12.0), "q=%r" % (coord(q),)
    return "Scale3d(2,3,4).getMatrix() 变换点(1,2,3)->(%+.3f,%+.3f,%+.3f) isProportional=%s" % (
        q.x, q.y, q.z, s.isProportional())


def s_extract():
    s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
    e = s.extractScale()
    return "extractScale -> (%+.3f,%+.3f,%+.3f)" % (e.sx, e.sy, e.sz)


mlab.check(L, M, "exists+construct", s_ctor)
mlab.check(L, M, "getMatrix_transformBy", s_matrix)
mlab.check(L, M, "extractScale", s_extract)
mlab.probe(L, M, "setScaleFactors 三 double 旧签名",
           lambda: Ge.AcGeScale3d().setScaleFactors(1.0, 1.0, 1.0), allow=("Exception",))

# ============================== Ge.AcGeLineSeg3d ==============================
L, M = "Ge", "AcGeLineSeg3d"


def l_ctor():
    a = Ge.AcGePoint3d(0.0, 0.0, 0.0)
    b = Ge.AcGePoint3d(3.0, 4.0, 0.0)
    ls = Ge.AcGeLineSeg3d(a, b)
    Ln = ls.length()
    mid = ls.midPoint()
    return "start(0,0,0)-end(3,4,0) length=%.6f mid=(%.3f,%.3f,%.3f) type=%s.%s" % (
        Ln, mid.x, mid.y, mid.z, type(ls).__module__, type(ls).__name__)


def l_endpoints():
    a = Ge.AcGePoint3d(1.0, 2.0, 0.0)
    b = Ge.AcGePoint3d(4.0, 6.0, 0.0)
    ls = Ge.AcGeLineSeg3d(a, b)
    s, e = ls.startPoint(), ls.endPoint()
    d = ls.direction()
    return "start=(%.1f,%.1f,%.1f) end=(%.1f,%.1f,%.1f) direction=(%.3f,%.3f,%.3f)" % (
        s.x, s.y, s.z, e.x, e.y, e.z, d.x, d.y, d.z)


def l_degenerate():
    p = Ge.AcGePoint3d(0.0, 0.0, 0.0)
    ls = Ge.AcGeLineSeg3d(p, p)
    return "重合端点 length=%.6f isDegenerate=%s" % (ls.length(), ls.isDegenerate())


mlab.check(L, M, "exists+construct", l_ctor)
mlab.check(L, M, "start_end_direction", l_endpoints)
mlab.check(L, M, "degenerate", l_degenerate)
mlab.probe(L, M, "ctor 点+向量形式",
           lambda: Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(1, 1, 1)),
           allow=("Exception",))

# ============================== Ge.AcGeCircArc3d ==============================
L, M = "Ge", "AcGeCircArc3d"


def c_ctor():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(1.0, 2.0, 0.0),
                         Ge.AcGeVector3d(0.0, 0.0, 1.0), 5.0)
    cn = c.center()
    return "center(1,2,0) normal z radius=%.3f center=(%.3f,%.3f,%.3f) type=%s.%s" % (
        c.radius(), cn.x, cn.y, cn.z, type(c).__module__, type(c).__name__)


def c_length():
    import math
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0.0, 0.0, 0.0),
                         Ge.AcGeVector3d(0.0, 0.0, 1.0), 2.0)
    Ln = c.length()
    expect = 2.0 * math.pi * 2.0
    return "整圆 r=2 length=%.6f 期望 2πr=%.6f isClosed=%s" % (Ln, expect, c.isClosed())


def c_setradius():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0.0, 0.0, 0.0),
                         Ge.AcGeVector3d(0.0, 0.0, 1.0), 1.0)
    c.setRadius(7.5)
    return "setRadius(7.5) 读回=%.3f" % c.radius()


mlab.check(L, M, "exists+construct", c_ctor)
mlab.check(L, M, "length_full_circle", c_length)
mlab.check(L, M, "setRadius_roundtrip", c_setradius)
mlab.probe(L, M, "负半径构造",
           lambda: Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0),
           allow=("Exception",))

# ============================== Ge.AcGePlane ==============================
L, M = "Ge", "AcGePlane"


def pl_ctor():
    pl = Ge.AcGePlane(Ge.AcGePoint3d(0.0, 0.0, 1.0), Ge.AcGeVector3d(0.0, 0.0, 1.0))
    n = pl.normal()
    return "origin(0,0,1) normal z -> normal=(%.3f,%.3f,%.3f) type=%s.%s" % (
        n.x, n.y, n.z, type(pl).__module__, type(pl).__name__)


def pl_coef():
    pl = Ge.AcGePlane(0.0, 0.0, 1.0, -2.0)  # z - 2 = 0
    coef = pl.coefficients()
    return "(a,b,c,d)=%r" % (list(coef),)


def pl_dist():
    pl = Ge.AcGePlane(0.0, 0.0, 1.0, 0.0)  # z=0
    d = pl.signedDistanceTo(Ge.AcGePoint3d(0.0, 0.0, 3.5))
    return "平面 z=0 点(0,0,3.5) signedDistanceTo=%.6f" % d


def pl_const():
    return "kXYPlane=%r kYZPlane=%r kZXPlane=%r" % (
        Ge.AcGePlane.kXYPlane is not None, Ge.AcGePlane.kYZPlane is not None,
        Ge.AcGePlane.kZXPlane is not None)


mlab.check(L, M, "exists+construct", pl_ctor)
mlab.check(L, M, "coefficients", pl_coef)
mlab.check(L, M, "signedDistanceTo", pl_dist)
mlab.check(L, M, "class_constant", pl_const)

# ============================== Gi.PyGiTextStyle ==============================
L, M = "Gi", "PyGiTextStyle"


def ts_ctor():
    t = Gi.PyGiTextStyle()
    return "无参构造成功 textSize=%.6f xScale=%.6f type=%s.%s" % (
        t.textSize(), t.xScale(), type(t).__module__, type(t).__name__)


def ts_roundtrip():
    t = Gi.PyGiTextStyle()
    t.setTextSize(5.25)
    t.setXScale(0.8)
    t.setObliquingAngle(15.0)
    t.setTrackingPercent(120.0)
    t.setStyleName("STANDARD")
    return "textSize=%.4f xScale=%.4f obliquingAngle=%.4f trackingPercent=%.4f styleName=%r" % (
        t.textSize(), t.xScale(), t.obliquingAngle(), t.trackingPercent(), t.styleName())


def ts_file():
    t = Gi.PyGiTextStyle()
    t.setFileName("gbenor.shx")
    return "setFileName(gbenor.shx) fileName=%r valid=%s" % (t.fileName(), t.valid())


def ts_bool():
    t = Gi.PyGiTextStyle()
    t.setUnderlined(True)
    t.setBackward(False)
    return "isUnderlined=%s isBackward=%s" % (t.isUnderlined(), t.isBackward())


def ts_dbtext_loop():
    db = P.Db.curDb()
    before = len(list(db.objectIds("AcDbEntity")))
    tx = P.Db.PyDbTrans()
    with tx:
        txt = P.Db.PyDbText(Ge.AcGePoint3d(10.0, 20.0, 0.0), "PyArx", 5.25, 0.0)
        tx.addEntity(txt)
        h = str(txt.handle())
        hgt = float(txt.height())
    after = len(list(db.objectIds("AcDbEntity")))
    assert h != "0" and after > before, "handle=%s before=%d after=%d" % (h, before, after)
    return "PyDbText(10,20,'PyArx',height=5.25) 入库 handle=%s height=%.4f 实体数 %d->%d" % (
        h, hgt, before, after)


mlab.check(L, M, "exists+construct", ts_ctor)
mlab.check(L, M, "trait_roundtrip", ts_roundtrip)
mlab.check(L, M, "fileName_valid", ts_file)
mlab.check(L, M, "bool_trait", ts_bool)
mlab.check(L, M, "dbtext_closed_loop", ts_dbtext_loop)
mlab.probe(L, M, "非法字体文件 setFileName",
           lambda: Gi.PyGiTextStyle().setFileName("nope.shx"), allow=("Exception", "False"))
mlab.probe(L, M, "setTextSize 传字符串",
           lambda: Gi.PyGiTextStyle().setTextSize("abc"), allow=("Exception",))

# ============ Gi 抽象类：存在性 + 成员 + 构造失败面（不可实例化） ============
ABSTRACT = ["PyGiDrawableTraits", "PyGiSubEntityTraits", "PyGiGeometry",
            "PyGiWorldGeometry", "PyGiDrawable", "PyGiWorldDraw"]

for name in ABSTRACT:
    cls = getattr(Gi, name, None)
    Lm = "Gi"
    mlab.check(Lm, name, "exists+members",
               lambda c=cls, n=name: "exists=%s members=%d ctor=%s" % (
                   c is not None, len([x for x in dir(c) if not x.startswith("_")]),
                   mlab.typeerror_hint(c, "x")[:160]))
    if cls is not None:
        mlab.probe(Lm, name, "无参构造", lambda c=cls: c(), allow=("Exception",))

# 抽象 Traits 方法级签名（在类上点错参数，抓 TypeError 重载原文）
for meth in ("setColor", "setLineWeight", "setLayer", "setTrueColor"):
    mlab.probe("Gi", "PyGiDrawableTraits", "method " + meth,
               lambda mm=meth: mlab.method_typeerror(Gi.PyGiDrawableTraits, mm, "x"),
               allow=("Exception",))

# ============================== Gi：三个不存在的名字 ==============================
for name, alt in [
    ("PyGiColor", "颜色走 PyGiDrawableTraits.setColor/setTrueColor 或 PyDbEntity.colorRGB；Gi 内真实类为 AcCmEntityColor、AcGiColorRGB、AcGiColorRGBA"),
    ("PyGiPolyline", "Gi 内真实折线类为 AcGiPolyline；绘制走 PyGiWorldGeometry.polyline/polylineObj/pline，不在 PyGi 前缀下"),
    ("PyGiContextGdi", "Gi 内上下文基类为 AcGiContext；GDI 绘制对象为 AcGiGdiDrawObject，无 PyGiContextGdi"),
]:
    exists = hasattr(Gi, name)
    mlab.check("Gi", name, "exists_missing",
               lambda n=name, a=alt, e=exists: "hasattr=%s 不存在；替代：%s" % (e, a))

# ============================== 附加扫描：清单外关键类 ==============================
def scan_ge():
    want = ["AcGeTol", "AcGeInterval", "AcGeMatrix2d", "AcGeScale2d", "AcGeLine2d"]
    return {n: hasattr(Ge, n) for n in want}


def scan_gi():
    want = ["AcCmEntityColor", "AcGiColorRGB", "AcGiColorRGBA", "AcGiPolyline",
            "AcGiContext", "AcGiGdiDrawObject", "PyGiTextEngine", "GiPythonDrawable"]
    return {n: hasattr(Gi, n) for n in want}


mlab.check("Ge", "extra_scan", "exists_scan", lambda: "Ge 清单外类存在性: %r" % scan_ge())
mlab.check("Gi", "extra_scan", "exists_scan", lambda: "Gi 清单外类存在性: %r" % scan_gi())

out = mlab.out_path(r"G:\PyArx\tools\train\data\shard_ge_gi.json")
rep = mlab.report()
with open(out, "w", encoding="utf-8") as f:
    json.dump(rep, f, ensure_ascii=False, indent=1)
mlab.dump(out)
print(json.dumps({"written": out, "modules": len(rep)}, ensure_ascii=False))
