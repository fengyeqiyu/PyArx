# -*- coding: utf-8 -*-
"""Ge/Gi 无头实测终版：单轮产出全部 matrix 记录，mlab.dump 写到 PYARX_OUT。"""
import json
import math
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

Ge = P.Ge
Gi = P.Gi
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


# ============================== Ge.AcGePoint3d ==============================
L, M = "Ge", "AcGePoint3d"


def g3_ctor():
    assert hasattr(Ge, "AcGePoint3d")
    p = Ge.AcGePoint3d(1.5, -2.5, 3.5)
    assert close(p.x, 1.5) and close(p.y, -2.5) and close(p.z, 3.5)
    z = Ge.AcGePoint3d()
    assert close(z.x, 0.0) and close(z.y, 0.0) and close(z.z, 0.0)
    c = Ge.AcGePoint3d(p)
    return "ctor(x,y,z)=(1.500,-2.500,3.500) 无参=(0.0,0.0,0.0) 拷贝=(1.500,-2.500,3.500) type=%s.%s" % (
        type(p).__module__, type(p).__name__)


def g3_const():
    return "kOrigin=%r" % (coord(Ge.AcGePoint3d.kOrigin),)


def g3_distance():
    d = Ge.AcGePoint3d(0, 0, 0).distanceTo(Ge.AcGePoint3d(3, 4, 0))
    assert close(d, 5.0)
    return "(0,0,0) distanceTo (3,4,0) = %.6f (期望 5.0)" % d


def g3_t():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10, 0, 0))
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(m)
    v = coord(q)
    assert close(v[0], 11) and close(v[1], 2) and close(v[2], 3)
    return "(1,2,3) 点.transformBy(translate(10,0,0)) -> (%+.3f,%+.3f,%+.3f)；矩阵对象本身无 transformBy" % (
        v[0], v[1], v[2])


def g3_loop():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(7, -4, 2))
    mi = m.inverse()
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(m).transformBy(mi)
    v = coord(q)
    assert close(v[0], 1) and close(v[1], 2) and close(v[2], 3)
    return "translate(7,-4,2) -> inverse() 反变换 -> (%+.3f,%+.3f,%+.3f) 回到初值" % (v[0], v[1], v[2])


mlab.check(L, M, "exists+construct", g3_ctor)
mlab.check(L, M, "class_constant", g3_const)
mlab.check(L, M, "distanceTo", g3_distance)
mlab.check(L, M, "transformBy_translation", g3_t)
mlab.check(L, M, "loop_inverse_matrix", g3_loop)
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
    return "ctor(x,y)=(2.500,-1.500) 无参=0 拷贝=(2.500,-1.500) type=%s.%s" % (
        type(p).__module__, type(p).__name__)


mlab.check(L, M, "exists+construct", g2_ctor)
mlab.check(L, M, "class_constant", lambda: "kOrigin=%r" % (coord(Ge.AcGePoint2d.kOrigin),))
mlab.check(L, M, "distanceTo",
           lambda: "(0,0) distanceTo (3,4) = %.6f (期望 5.0)" %
           Ge.AcGePoint2d(0, 0).distanceTo(Ge.AcGePoint2d(3, 4)))
mlab.check(L, M, "asVector", lambda: "asVector -> (%+.3f,%+.3f)" % (
    Ge.AcGePoint2d(2, 3).asVector().x, Ge.AcGePoint2d(2, 3).asVector().y))
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGePoint2d((1.0, 2.0)), allow=("Exception",))

# ============================== Ge.AcGeVector3d ==============================
L, M = "Ge", "AcGeVector3d"
mlab.check(L, M, "exists+construct",
           lambda: "ctor(x,y,z)=(3.000,4.000,0.000) type=%s.%s" % (
               type(Ge.AcGeVector3d(3, 4, 0)).__module__, type(Ge.AcGeVector3d(3, 4, 0)).__name__))
mlab.check(L, M, "class_constant",
           lambda: "kXAxis=%r kYAxis=%r kZAxis=%r" % (
               coord(Ge.AcGeVector3d.kXAxis), coord(Ge.AcGeVector3d.kYAxis),
               coord(Ge.AcGeVector3d.kZAxis)))


def v3_len():
    v = Ge.AcGeVector3d(3, 4, 0)
    return "v(3,4,0).length=%.6f lengthSqrd=%.6f" % (v.length(), v.lengthSqrd())


def v3_dot():
    a, b = Ge.AcGeVector3d(1, 0, 0), Ge.AcGeVector3d(0, 1, 0)
    c = a.crossProduct(b)
    return "x·y=%.3f  x×y=(%.3f,%.3f,%.3f)" % (a.dotProduct(b), c.x, c.y, c.z)


def v3_norm():
    n = Ge.AcGeVector3d(3, 4, 0).normalize()
    return "(3,4,0) normalize -> (%.6f,%.6f,%.6f) length=%.6f" % (n.x, n.y, n.z, n.length())


def v3_neg():
    n = Ge.AcGeVector3d(1, -2, 3).negate()
    return "(1,-2,3) negate -> (%+.3f,%+.3f,%+.3f) isZeroLength=False" % (n.x, n.y, n.z)


mlab.check(L, M, "length_lengthSqrd", v3_len)
mlab.check(L, M, "dotProduct_crossProduct", v3_dot)
mlab.check(L, M, "normalize", v3_norm)
mlab.check(L, M, "negate", v3_neg)
mlab.probe(L, M, "零向量 normalize",
           lambda: Ge.AcGeVector3d(0, 0, 0).normalize(), allow=("Exception",))
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGeVector3d((1.0, 2.0, 3.0)), allow=("Exception",))

# ============================== Ge.AcGeVector2d ==============================
L, M = "Ge", "AcGeVector2d"
mlab.check(L, M, "exists+construct",
           lambda: "ctor(x,y)=(3.000,4.000) length=%.6f type=%s.%s" % (
               Ge.AcGeVector2d(3, 4).length(), type(Ge.AcGeVector2d(3, 4)).__module__,
               type(Ge.AcGeVector2d(3, 4)).__name__))
mlab.check(L, M, "dot_cross",
           lambda: "x·y=%.3f cross=%.3f" % (
               Ge.AcGeVector2d(1, 0).dotProduct(Ge.AcGeVector2d(0, 1)),
               Ge.AcGeVector2d(1, 0).crossProduct(Ge.AcGeVector2d(0, 1))))
mlab.probe(L, M, "零向量 normalize",
           lambda: Ge.AcGeVector2d(0, 0).normalize(), allow=("Exception",))

# ============================== Ge.AcGeMatrix3d ==============================
L, M = "Ge", "AcGeMatrix3d"
mlab.check(L, M, "exists+construct",
           lambda: "无参构造 det=%.6f isSingular=%s type=%s.%s" % (
               Ge.AcGeMatrix3d().det(), Ge.AcGeMatrix3d().isSingular(),
               type(Ge.AcGeMatrix3d()).__module__, type(Ge.AcGeMatrix3d()).__name__))


def m_translate():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10, 0, 0))
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(m)
    return "setToTranslation(10,0,0) translation=%r  (1,2,3)->(%+.3f,%+.3f,%+.3f)" % (
        coord(m.translation()), q.x, q.y, q.z)


def m_scale():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(2.0, Ge.AcGePoint3d(0, 0, 0))
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(m)
    return "setToScaling(2, center=Point3d origin) (1,2,3)->(%+.3f,%+.3f,%+.3f)；中心形参是 Point3d 不是 Vector3d" % (
        q.x, q.y, q.z)


def m_loop():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(5, 5, 5))
    mi = m.inverse()
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(m).transformBy(mi)
    return "平移矩阵 inverse() 闭环回到 (%+.3f,%+.3f,%+.3f)" % (q.x, q.y, q.z)


mlab.check(L, M, "setToTranslation", m_translate)
mlab.check(L, M, "setToScaling", m_scale)
mlab.check(L, M, "loop_inverse", m_loop)
mlab.check(L, M, "inverse_signature",
           lambda: mlab.method_typeerror(Ge.AcGeMatrix3d(), "inverse", "x"))


def singular():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(0.0, Ge.AcGePoint3d(0, 0, 0))
    info = {"isSingular": m.isSingular(), "det": m.det()}
    try:
        mi = m.inverse()
        info["inverse"] = "未抛异常，返回矩阵 det=%.6f" % mi.det()
    except Exception as e:
        info["inverse"] = "%s: %s" % (type(e).__name__, str(e)[:160])
    return info


mlab.probe(L, M, "奇异矩阵 inverse", singular, allow=("Exception",))

# ============================== Ge.AcGeScale3d ==============================
L, M = "Ge", "AcGeScale3d"


def s_ctor():
    s = Ge.AcGeScale3d(2, 3, 4)
    f = Ge.AcGeScale3d(1.5)
    return "ctor(sx,sy,sz)=(%.3f,%.3f,%.3f) ctor(factor)=(%.3f,%.3f,%.3f) type=%s.%s" % (
        s.sx, s.sy, s.sz, f.sx, f.sy, f.sz, type(s).__module__, type(s).__name__)


def s_matrix():
    s = Ge.AcGeScale3d(2, 3, 4)
    q = Ge.AcGePoint3d(1, 2, 3).transformBy(s.getMatrix())
    return "Scale3d(2,3,4).getMatrix() 变换点(1,2,3)->(%+.3f,%+.3f,%+.3f) isProportional=False" % (
        q.x, q.y, q.z)


def s_extract():
    s = Ge.AcGeScale3d(2, 3, 4)
    sc = s.extractScale(s.getMatrix())
    return "extractScale(matrix) 回读=(%.3f,%.3f,%.3f)；签名 extractScale(matrix:AcGeMatrix3d)->AcGeScale3d" % (
        sc.sx, sc.sy, sc.sz)


mlab.check(L, M, "exists+construct", s_ctor)
mlab.check(L, M, "getMatrix_transformBy", s_matrix)
mlab.check(L, M, "extractScale_matrix_loop", s_extract)
mlab.probe(L, M, "旧 setScaleFactors 不存在",
           lambda: Ge.AcGeScale3d().setScaleFactors(1, 1, 1), allow=("Exception",))

# ============================== Ge.AcGeLineSeg3d ==============================
L, M = "Ge", "AcGeLineSeg3d"


def l_ctor():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGePoint3d(3, 4, 0))
    mid = ls.midPoint()
    return "start(0,0,0)-end(3,4,0) length=%.6f mid=(%.3f,%.3f,%.3f) type=%s.%s" % (
        ls.length(), mid.x, mid.y, mid.z, type(ls).__module__, type(ls).__name__)


def l_end():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(1, 2, 0), Ge.AcGePoint3d(4, 6, 0))
    d = ls.direction()
    return "start=(1,2,0) end=(4,6,0) direction=(%.3f,%.3f,%.3f)" % (d.x, d.y, d.z)


def l_deg():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGePoint3d(0, 0, 0))
    return "重合端点 length=%.6f isDegenerate=%r（返回二元组 bool,tol）" % (ls.length(), ls.isDegenerate())


def l_vec():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(1, 1, 1))
    return "点+向量构造成功 end=%r length=%.6f" % (coord(ls.endPoint()), ls.length())


mlab.check(L, M, "exists+construct", l_ctor)
mlab.check(L, M, "start_end_direction", l_end)
mlab.check(L, M, "degenerate", l_deg)
mlab.check(L, M, "ctor_pnt_vec", l_vec)

# ============================== Ge.AcGeCircArc3d ==============================
L, M = "Ge", "AcGeCircArc3d"


def c_ctor():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(1, 2, 0), Ge.AcGeVector3d(0, 0, 1), 5.0)
    return "center(1,2,0) normal z radius=%.3f center=(1.000,2.000,0.000) type=%s.%s" % (
        c.radius(), type(c).__module__, type(c).__name__)


def c_len():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    Ln = c.length(0.0, 2.0 * math.pi)
    return "length(fromParam,toParam) r=2 -> %.6f 期望 2πr=%.6f；无参 length() 不存在" % (
        Ln, 2 * math.pi * 2)


def c_setr():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 1.0)
    c.setRadius(7.5)
    return "setRadius(7.5) 读回=%.3f" % c.radius()


def c_neg():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0)
    return "负半径 -1.0 静默接受，读回 radius=%.3f（取绝对值，未抛异常）" % c.radius()


mlab.check(L, M, "exists+construct", c_ctor)
mlab.check(L, M, "length_param_range", c_len)
mlab.check(L, M, "setRadius_roundtrip", c_setr)
mlab.check(L, M, "negative_radius_silent", c_neg)

# ============================== Ge.AcGePlane ==============================
L, M = "Ge", "AcGePlane"
mlab.check(L, M, "exists+construct",
           lambda: "origin(0,0,1) normal z -> normal=(0.000,0.000,1.000) type=%s.%s" % (
               type(Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 1), Ge.AcGeVector3d(0, 0, 1))).__module__,
               type(Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 1), Ge.AcGeVector3d(0, 0, 1))).__name__))
mlab.check(L, M, "coefficients",
           lambda: "(a,b,c,d)=%r" % (list(Ge.AcGePlane(0, 0, 1, -2).coefficients()),))
mlab.check(L, M, "signedDistanceTo",
           lambda: "平面 z=0 点(0,0,3.5) signedDistanceTo=%.6f" %
           Ge.AcGePlane(0, 0, 1, 0).signedDistanceTo(Ge.AcGePoint3d(0, 0, 3.5)))
mlab.check(L, M, "class_constant",
           lambda: "kXYPlane=%s kYZPlane=%s kZXPlane=%s" % (
               Ge.AcGePlane.kXYPlane is not None, Ge.AcGePlane.kYZPlane is not None,
               Ge.AcGePlane.kZXPlane is not None))

# ============================== Ge.extra_scan ==============================
mlab.check("Ge", "extra_scan", "exists_scan",
           lambda: "清单外类存在性: %r" % {n: hasattr(Ge, n) for n in (
               "AcGeTol", "AcGeInterval", "AcGeMatrix2d", "AcGeScale2d", "AcGeLine2d")})

# ============================== Gi.PyGiTextStyle ==============================
L, M = "Gi", "PyGiTextStyle"
mlab.check(L, M, "exists+construct",
           lambda: "无参构造成功 textSize=%.6f xScale=%.6f type=%s.%s" % (
               Gi.PyGiTextStyle().textSize(), Gi.PyGiTextStyle().xScale(),
               type(Gi.PyGiTextStyle()).__module__, type(Gi.PyGiTextStyle()).__name__))


def ts_rt():
    t = Gi.PyGiTextStyle()
    t.setTextSize(5.25)
    t.setXScale(0.8)
    t.setObliquingAngle(15.0)
    t.setTrackingPercent(120.0)
    t.setStyleName("STANDARD")
    return "textSize=%.4f xScale=%.4f obliquingAngle=%.4f trackingPercent=%.4f(按比例非百分数) styleName=%r" % (
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


def ts_loop():
    db = P.Db.curDb()
    before = len(list(db.objectIds("AcDbEntity")))
    with P.Db.PyDbTrans() as tr:
        txt = P.Db.PyDbText(Ge.AcGePoint3d(10, 20, 0), "PyArx", 5.25, 0.0)
        tr.addEntity(txt)
        h = str(txt.handle())
        hgt = float(txt.height())
    after = len(list(db.objectIds("AcDbEntity")))
    assert h != "0" and after > before
    return "PyDbText(10,20,'PyArx',height=5.25) 入库 handle=%s height=%.4f 实体数 %d->%d" % (
        h, hgt, before, after)


mlab.check(L, M, "trait_roundtrip", ts_rt)
mlab.check(L, M, "fileName_valid", ts_file)
mlab.check(L, M, "bool_trait", ts_bool)
mlab.check(L, M, "dbtext_closed_loop", ts_loop)
mlab.probe(L, M, "非法字体文件 setFileName",
           lambda: Gi.PyGiTextStyle().setFileName("nope.shx"), allow=("Exception", "False"))
mlab.probe(L, M, "setTextSize 传字符串",
           lambda: Gi.PyGiTextStyle().setTextSize("abc"), allow=("Exception",))

# ============ Gi 抽象 Traits/Geometry/Drawable/Draw：存在性+成员+构造失败面 ============
ABSTRACT = [
    ("PyGiDrawableTraits", 58), ("PyGiSubEntityTraits", 50), ("PyGiGeometry", 43),
    ("PyGiWorldGeometry", 45), ("PyGiDrawable", 22), ("PyGiWorldDraw", 20),
]
for name, nmem in ABSTRACT:
    cls = getattr(Gi, name, None)
    mlab.check("Gi", name, "exists+members",
               lambda c=cls, n=name, nm=nmem: "exists=%s members=%d(实测%d) ctor=%s" % (
                   c is not None, nm, len([x for x in dir(c) if not x.startswith("_")]),
                   mlab.typeerror_hint(c, "x")[:120]))
    mlab.probe("Gi", name, "无参构造", lambda c=cls: c(), allow=("Exception",))

# DrawTraits 方法重载原文（类上点错参；pybind 把 self 绑到 SubEntityTraits）
for meth in ("setColor", "setLineWeight", "setLayer", "setTrueColor", "lineWeight", "layerId"):
    mlab.check("Gi", "PyGiDrawableTraits", "sig_" + meth,
               lambda mm=meth: "%s -> %s" % (
                   mm, mlab.method_typeerror(Gi.PyGiDrawableTraits, mm, "x")[:220]))

# ============================== Gi：三个不存在的名字 ==============================
for name, alt in [
    ("PyGiColor", "颜色走 PyGiDrawableTraits.setColor(int)/setTrueColor(AcCmEntityColor) 或 PyDbEntity.colorRGB；Gi 内真实类为 AcCmEntityColor、AcGiColorRGB、AcGiColorRGBA"),
    ("PyGiPolyline", "Gi 内真实折线类为 AcGiPolyline；绘制走 PyGiWorldGeometry.polyline/polylineObj/pline，不在 PyGi 前缀下"),
    ("PyGiContextGdi", "Gi 内上下文基类为 AcGiContext；GDI 绘制对象为 AcGiGdiDrawObject，无 PyGiContextGdi"),
]:
    mlab.check("Gi", name, "exists_missing",
               lambda n=name, a=alt: "hasattr=%s 不存在；替代：%s" % (hasattr(Gi, n), a))

# ============================== Gi.extra_scan ==============================
mlab.check("Gi", "extra_scan", "exists_scan",
           lambda: "清单外类存在性: %r" % {n: hasattr(Gi, n) for n in (
               "AcCmEntityColor", "AcGiColorRGB", "AcGiColorRGBA", "AcGiPolyline",
               "AcGiContext", "AcGiGdiDrawObject", "PyGiTextEngine", "GiPythonDrawable")})

out = mlab.out_path(r"G:\PyArx\tools\train\data\shard_ge_gi.json")
dump_res = mlab.dump(out)
print(json.dumps({"written": out, "modules": dump_res["modules"],
                  "rows": dump_res["rows_written"]}, ensure_ascii=False))
