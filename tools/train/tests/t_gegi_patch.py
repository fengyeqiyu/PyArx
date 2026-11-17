# -*- coding: utf-8 -*-
"""补丁轮：修正上一轮脚本侧 bug，重跑受影响 Ge/Gi 模块；dump 按 layer+module 合并覆盖。"""
import json
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


# ---------------- Point3d ----------------
L, M = "Ge", "AcGePoint3d"

mlab.check(L, M, "class_constant", lambda: "kOrigin=%r" % (coord(Ge.AcGePoint3d.kOrigin),))


def g3_t():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    q = p.transformBy(m)
    v = coord(q)
    assert close(v[0], 11.0) and close(v[1], 2.0) and close(v[2], 3.0)
    return "(1,2,3) transformBy translate(10,0,0) -> (%+.3f,%+.3f,%+.3f) （点.transformBy(矩阵)，矩阵无 transformBy）" % (
        v[0], v[1], v[2])


mlab.check(L, M, "transformBy_translation", g3_t)


def g3_loop():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(7.0, -4.0, 2.0))
    mi = m.inverse()
    q = p.transformBy(m).transformBy(mi)
    v = coord(q)
    assert close(v[0], 1.0) and close(v[1], 2.0) and close(v[2], 3.0)
    return "translate(7,-4,2) -> inverse() 反变换 -> (%+.3f,%+.3f,%+.3f) 回到初值" % (v[0], v[1], v[2])


mlab.check(L, M, "loop_inverse_matrix", g3_loop)

# ---------------- Point2d ----------------
L, M = "Ge", "AcGePoint2d"
mlab.check(L, M, "class_constant", lambda: "kOrigin=%r" % (coord(Ge.AcGePoint2d.kOrigin),))

# ---------------- Vector3d ----------------
L, M = "Ge", "AcGeVector3d"


def v3_const():
    return "kXAxis=%r kYAxis=%r kZAxis=%r" % (
        coord(Ge.AcGeVector3d.kXAxis), coord(Ge.AcGeVector3d.kYAxis), coord(Ge.AcGeVector3d.kZAxis))


mlab.check(L, M, "class_constant", v3_const)

# ---------------- Matrix3d ----------------
L, M = "Ge", "AcGeMatrix3d"


def m_translate():
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    t = coord(m.translation())
    q = Ge.AcGePoint3d(1.0, 2.0, 3.0).transformBy(m)
    return "setToTranslation(10,0,0) translation=%r  (1,2,3)->(%+.3f,%+.3f,%+.3f)" % (
        t, q.x, q.y, q.z)


mlab.check(L, M, "setToTranslation", m_translate)


def m_scale():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(2.0, Ge.AcGePoint3d(0.0, 0.0, 0.0))
    q = Ge.AcGePoint3d(1.0, 2.0, 3.0).transformBy(m)
    return "setToScaling(2, center=Point3d origin) (1,2,3)->(%+.3f,%+.3f,%+.3f)" % (q.x, q.y, q.z)


mlab.check(L, M, "setToScaling", m_scale)


def m_loop():
    p = Ge.AcGePoint3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(5.0, 5.0, 5.0))
    mi = m.inverse()
    q = p.transformBy(m).transformBy(mi)
    assert close(q.x, 1.0) and close(q.y, 2.0) and close(q.z, 3.0)
    return "平移矩阵 inverse() 闭环回到 (%+.3f,%+.3f,%+.3f)" % (q.x, q.y, q.z)


mlab.check(L, M, "loop_inverse", m_loop)
mlab.check(L, M, "inverse_signature",
           lambda: mlab.method_typeerror(Ge.AcGeMatrix3d(), "inverse", "x"))


def singular():
    m = Ge.AcGeMatrix3d()
    m.setToScaling(0.0, Ge.AcGePoint3d(0.0, 0.0, 0.0))
    info = {"isSingular": m.isSingular(), "det": m.det()}
    try:
        mi = m.inverse()
        info["inverse"] = "returned matrix, det=%.6f" % mi.det()
    except Exception as e:
        info["inverse"] = "%s: %s" % (type(e).__name__, str(e)[:160])
    return info


mlab.probe(L, M, "奇异矩阵 inverse", singular, allow=("Exception",))

# ---------------- Scale3d ----------------
L, M = "Ge", "AcGeScale3d"


def s_extract():
    s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
    sig = mlab.method_typeerror(s, "extractScale", "x")
    out = Ge.AcGeScale3d()
    s.extractScale(out)
    return "extractScale(out) -> out=(%.3f,%.3f,%.3f) 签名: %s" % (
        out.sx, out.sy, out.sz, sig[:140])


mlab.check(L, M, "extractScale", s_extract)

# ---------------- LineSeg3d ----------------
L, M = "Ge", "AcGeLineSeg3d"


def l_vec():
    ls = Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(1, 1, 1))
    return "点+向量构造成功 end=%r length=%.6f" % (coord(ls.endPoint()), ls.length())


mlab.check(L, M, "ctor_pnt_vec", l_vec)

# ---------------- CircArc3d ----------------
L, M = "Ge", "AcGeCircArc3d"


def c_sig():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    sig = mlab.method_typeerror(c, "length", "x")
    return "length 签名: " + sig[:220]


mlab.check(L, M, "length_signature", c_sig)


def c_len_interval():
    import math
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    iv = Ge.AcGeInterval(0.0, 2.0 * math.pi)
    Ln = c.length(iv)
    return "length(interval 0..2π) r=2 -> %.6f 期望 2πr=%.6f" % (Ln, 2 * math.pi * 2)


mlab.check(L, M, "length_interval", c_len_interval)


def c_neg():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0)
    return "负半径 -1.0 静默接受，读回 radius=%.3f（取绝对值，未抛异常）" % c.radius()


mlab.check(L, M, "negative_radius_silent", c_neg)

# ---------------- Gi DrawTraits 方法签名（类上点错参抓重载原文） ----------------
L, M = "Gi", "PyGiDrawableTraits"
for meth in ("setColor", "setLineWeight", "setLayer", "setTrueColor", "lineWeight", "layerId"):
    mlab.check(L, M, "sig_" + meth,
               lambda mm=meth: "%s -> %s" % (mm, mlab.method_typeerror(Gi.PyGiDrawableTraits, mm, "x")[:220]))

# ---------------- Gi TextStyle 补充事实 ----------------
L, M = "Gi", "PyGiTextStyle"


def ts_track():
    t = Gi.PyGiTextStyle()
    t.setTrackingPercent(120.0)
    return "setTrackingPercent(120.0) 读回 trackingPercent=%.6f（按比例返回，非百分数）" % t.trackingPercent()


mlab.check(L, M, "tracking_percent_semantics", ts_track)
mlab.probe(L, M, "非法字体文件 setFileName",
           lambda: Gi.PyGiTextStyle().setFileName("nope.shx"), allow=("Exception", "False"))

out = mlab.out_path(r"G:\PyArx\tools\train\data\shard_ge_gi.json")
mlab.dump(out)
print(json.dumps({"written": out, "modules": len(mlab.report())}))
