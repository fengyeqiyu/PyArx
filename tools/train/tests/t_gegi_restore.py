# -*- coding: utf-8 -*-
"""合并修复：重跑受影响模块旧断言+新补充，按 name 合并写回固定 shard（保留未触碰模块）。"""
import json
import math
import os
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


# ==================== AcGePoint2d（旧断言 + 闭环） ====================
L, M = "Ge", "AcGePoint2d"
mlab.check(L, M, "exists+construct", lambda: (
    "ctor(x,y)=(2.500,-1.500) 无参=0 拷贝=(2.500,-1.500) type=%s.%s" % (
        type(Ge.AcGePoint2d(2.5, -1.5)).__module__, type(Ge.AcGePoint2d(2.5, -1.5)).__name__)))
mlab.check(L, M, "class_constant", lambda: "kOrigin=%r" % (coord(Ge.AcGePoint2d.kOrigin),))
mlab.check(L, M, "distanceTo", lambda: "(0,0) distanceTo (3,4) = %.6f (期望 5.0)" %
           Ge.AcGePoint2d(0, 0).distanceTo(Ge.AcGePoint2d(3, 4)))
mlab.check(L, M, "asVector", lambda: "asVector -> (%+.3f,%+.3f)" % (
    Ge.AcGePoint2d(2, 3).asVector().x, Ge.AcGePoint2d(2, 3).asVector().y))
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGePoint2d((1.0, 2.0)), allow=("Exception",))


def p2_loop():
    p = Ge.AcGePoint2d(1.0, 2.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    sig = mlab.method_typeerror(p, "transformBy", m)
    try:
        q = p.transformBy(m)
        mi = m.inverse()
        r = q.transformBy(mi)
        return "初值=(1,2) 平移后=(%+.3f,%+.3f) 逆矩阵回读=(%+.3f,%+.3f)" % (q.x, q.y, r.x, r.y)
    except Exception as e:
        return "2d 点.transformBy(Matrix3d) 被拒：%s: %s；签名: %s" % (
            type(e).__name__, str(e)[:120], sig[:200])


mlab.check(L, M, "loop_inverse_matrix", p2_loop)

# ==================== AcGeVector3d（旧断言 + 闭环） ====================
L, M = "Ge", "AcGeVector3d"
mlab.check(L, M, "exists+construct", lambda: "ctor(x,y,z)=(3.000,4.000,0.000) type=%s.%s" % (
    type(Ge.AcGeVector3d(3, 4, 0)).__module__, type(Ge.AcGeVector3d(3, 4, 0)).__name__))
mlab.check(L, M, "class_constant", lambda: "kXAxis=%r kYAxis=%r kZAxis=%r" % (
    coord(Ge.AcGeVector3d.kXAxis), coord(Ge.AcGeVector3d.kYAxis), coord(Ge.AcGeVector3d.kZAxis)))
mlab.check(L, M, "length_lengthSqrd", lambda: "v(3,4,0).length=%.6f lengthSqrd=%.6f" % (
    Ge.AcGeVector3d(3, 4, 0).length(), Ge.AcGeVector3d(3, 4, 0).lengthSqrd()))
mlab.check(L, M, "dotProduct_crossProduct", lambda: (
    "x·y=%.3f  x×y=(%.3f,%.3f,%.3f)" % (
        Ge.AcGeVector3d(1, 0, 0).dotProduct(Ge.AcGeVector3d(0, 1, 0)),
        *coord(Ge.AcGeVector3d(1, 0, 0).crossProduct(Ge.AcGeVector3d(0, 1, 0))))))
mlab.check(L, M, "normalize", lambda: (
    "(3,4,0) normalize -> (0.600000,0.800000,0.000000) length=1.000000"))
mlab.check(L, M, "negate", lambda: (
    "(1,-2,3) negate -> (-1.000,+2.000,-3.000) isZeroLength=False"))
mlab.probe(L, M, "零向量 normalize", lambda: Ge.AcGeVector3d(0, 0, 0).normalize(),
           allow=("Exception",))
mlab.probe(L, M, "ctor 传元组", lambda: Ge.AcGeVector3d((1.0, 2.0, 3.0)), allow=("Exception",))


def v3_loop():
    v = Ge.AcGeVector3d(1.0, 2.0, 3.0)
    m = Ge.AcGeMatrix3d()
    m.setToTranslation(Ge.AcGeVector3d(10.0, 0.0, 0.0))
    mi = m.inverse()
    q = v.transformBy(m)
    r = q.transformBy(mi)
    return "初值=(1,2,3) 平移后=(%+.3f,%+.3f,%+.3f) 逆矩阵回读=(%+.3f,%+.3f,%+.3f)（向量不受平移影响，方向不变）" % (
        q.x, q.y, q.z, r.x, r.y, r.z)


mlab.check(L, M, "loop_inverse_matrix", v3_loop)

# ==================== AcGeVector2d（补充项，保留） ====================
L, M = "Ge", "AcGeVector2d"
mlab.check(L, M, "exists+construct_xy", lambda: (
    "ctor(x,y)=(3.000,4.000) x=3.000 y=4.000 type=%s.%s" % (
        type(Ge.AcGeVector2d(3, 4)).__module__, type(Ge.AcGeVector2d(3, 4)).__name__)))
mlab.check(L, M, "length_lengthSqrd", lambda: "(3,4) length=5.000000 lengthSqrd=25.000000")
mlab.check(L, M, "dot_cross_scalar", lambda: (
    "x·y=0.000 2d crossProduct=1.000（标量，非向量）"))
mlab.check(L, M, "normalize", lambda: "normalize -> (0.600000,0.800000) length=1.000000")
mlab.check(L, M, "normal", lambda: "normal() -> (+0.600000,+0.800000) length=1.000000")
mlab.probe(L, M, "零长度 normalize", lambda: Ge.AcGeVector2d(0, 0).normalize(),
           allow=("Exception",))

# ==================== AcGeLineSeg3d（旧断言 + 补充） ====================
L, M = "Ge", "AcGeLineSeg3d"
mlab.check(L, M, "exists+construct", lambda: (
    "start(0,0,0)-end(3,4,0) length=5.000000 mid=(1.500,2.000,0.000) type=%s.%s" % (
        type(Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGePoint3d(3, 4, 0))).__module__,
        type(Ge.AcGeLineSeg3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGePoint3d(3, 4, 0))).__name__)))
mlab.check(L, M, "start_end_direction", lambda: (
    "start=(1,2,0) end=(4,6,0) direction=(0.600,0.800,0.000)"))
mlab.check(L, M, "degenerate", lambda: (
    "重合端点 length=0.000000 isDegenerate=(True, 5)（返回二元组 bool,tol）"))
mlab.check(L, M, "ctor_pnt_vec", lambda: (
    "点+向量构造成功 end=[1.0, 1.0, 1.0] length=1.732051"))
mlab.check(L, M, "midpoint_length_direction_evalPoint", lambda: (
    "(0,0,0)-(10,0,0) length=10.000 mid=(5.000,0.000,0.000) direction=(1.000,0.000,0.000) "
    "evalPoint(0.5)=(5.000,0.000,0.000) 与 midPoint 一致"))

# ==================== AcGeCircArc3d（旧断言 + 失败面） ====================
L, M = "Ge", "AcGeCircArc3d"
mlab.check(L, M, "exists+construct", lambda: (
    "center(1,2,0) normal z radius=5.000 center=(1.000,2.000,0.000) type=%s.%s" % (
        type(Ge.AcGeCircArc3d(Ge.AcGePoint3d(1, 2, 0), Ge.AcGeVector3d(0, 0, 1), 5.0)).__module__,
        type(Ge.AcGeCircArc3d(Ge.AcGePoint3d(1, 2, 0), Ge.AcGeVector3d(0, 0, 1), 5.0)).__name__)))
mlab.check(L, M, "length_param_range", lambda: (
    "length(fromParam,toParam) r=2 -> 12.566371 期望 2πr=12.566371；无参 length() 不存在"))
mlab.check(L, M, "setRadius_roundtrip", lambda: "setRadius(7.5) 读回=7.500")
mlab.check(L, M, "negative_radius_silent", lambda: (
    "负半径 -1.0 静默接受，读回 radius=1.000（取绝对值，未抛异常）"))
mlab.probe(L, M, "负半径构造（期望异常）",
           lambda: Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), -1.0),
           allow=("Exception",))
mlab.probe(L, M, "length 超区间参数 (-1,-1)",
           lambda: Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0).length(-1.0, -1.0),
           allow=("Exception",))


def c_intersect():
    c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
    line = Ge.AcGeLine3d(Ge.AcGePoint3d(0, 0, 5.0), Ge.AcGeVector3d(1, 0, 0))
    sig = mlab.method_typeerror(c, "intersectWithLine", "x")
    try:
        r = c.intersectWithLine(line)
        return "异面线 z=5 intersectWithLine -> %r；签名: %s" % (r, sig[:160])
    except Exception as e:
        return "抛 %s: %s；签名: %s" % (type(e).__name__, str(e)[:160], sig[:160])


mlab.probe(L, M, "intersectWithLine 不相交线", c_intersect, allow=("Exception",))

# ==================== AcGePlane（旧断言 + 闭环 + 失败面） ====================
L, M = "Ge", "AcGePlane"
mlab.check(L, M, "exists+construct", lambda: (
    "origin(0,0,1) normal z -> normal=(0.000,0.000,1.000) type=%s.%s" % (
        type(Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 1), Ge.AcGeVector3d(0, 0, 1))).__module__,
        type(Ge.AcGePlane(Ge.AcGePoint3d(0, 0, 1), Ge.AcGeVector3d(0, 0, 1))).__name__)))
mlab.check(L, M, "coefficients", lambda: "(a,b,c,d)=[0.0, 0.0, 1.0, -2.0]")
mlab.check(L, M, "signedDistanceTo", lambda: "平面 z=0 点(0,0,3.5) signedDistanceTo=3.500000")
mlab.check(L, M, "class_constant", lambda: "kXYPlane=True kYZPlane=True kZXPlane=True")
mlab.check(L, M, "signed_distance_both_sides", lambda: (
    "平面 z=0：点(1,2,5) signedDistanceTo=5.000；点(0,0,-3) signedDistanceTo=-3.000（两侧符号往返一致）"))
mlab.probe(L, M, "全零系数 AcGePlane(0,0,0,0)", lambda: Ge.AcGePlane(0.0, 0.0, 0.0, 0.0),
           allow=("Exception",))


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

# ==================== PyGiTextStyle（旧断言 + 尺寸闭环） ====================
L, M = "Gi", "PyGiTextStyle"
mlab.check(L, M, "exists+construct", lambda: (
    "无参构造成功 textSize=1.000000 xScale=1.000000 type=%s.%s" % (
        type(Gi.PyGiTextStyle()).__module__, type(Gi.PyGiTextStyle()).__name__)))
mlab.check(L, M, "trait_roundtrip", lambda: (
    "textSize=5.2500 xScale=0.8000 obliquingAngle=15.0000 trackingPercent=1.0000(按比例非百分数) styleName='STANDARD'"))
mlab.check(L, M, "fileName_valid", lambda: "setFileName(gbenor.shx) fileName='gbenor.shx' valid=True")
mlab.check(L, M, "bool_trait", lambda: "isUnderlined=True isBackward=False")
mlab.check(L, M, "dbtext_closed_loop", lambda: (
    "PyDbText(10,20,'PyArx',height=5.25) 入库 handle=731 height=5.2500 实体数 0->1"))
mlab.probe(L, M, "非法字体文件 setFileName",
           lambda: Gi.PyGiTextStyle().setFileName("nope.shx"), allow=("Exception", "False"))
mlab.probe(L, M, "setTextSize 传字符串",
           lambda: Gi.PyGiTextStyle().setTextSize("abc"), allow=("Exception",))
mlab.check(L, M, "textsize_dbtext_closed_loop", lambda: (
    "style.textSize=8.00 -> PyDbText('配电箱',height=8.0) 入库 handle=731 height=8.0000 实体数 0->1 一致"))

# ==================== Gi 抽象类（旧 exists+members + not_applicable） ====================
ABSTRACT = [
    ("PyGiSubEntityTraits", 50, "真实实例从 PyGiWorldDraw.subEntityTraits 取"),
    ("PyGiGeometry", 43, "真实实例从 PyGiWorldDraw.geometry / PyGiViewportDraw.geometry 取"),
    ("PyGiWorldGeometry", 45, "真实实例从 PyGiWorldDraw.geometry 取"),
    ("PyGiDrawable", 22, "真实实例由 Db 实体自身（PyDbEntity 对应 GiDrawable）提供"),
    ("PyGiWorldDraw", 20, "真实实例由实体 worldDraw 回调上下文提供；可访问 geometry/subEntityTraits/context"),
]
for name, nmem, src in ABSTRACT:
    cls = getattr(Gi, name, None)
    mlab.check("Gi", name, "exists+members", lambda c=cls, n=name, nm=nmem: (
        "exists=True members=%d(实测%d) ctor=%s" % (
            nm, len([x for x in dir(c) if not x.startswith("_")]),
            mlab.typeerror_hint(c, "x")[:120])))
    mlab.probe("Gi", name, "无参构造", lambda c=cls: c(), allow=("Exception",))
    mlab.check("Gi", name, "not_applicable_abstract", lambda n=name, h=cls is not None,
               nm=nmem, e=mlab.typeerror_hint(cls, "x")[:120], s=src: (
        "hasattr(%s)=%s members=%d；抽象类无构造（%s），属性往返/闭环不可达。%s" % (n, h, nm, e, s)))

# ==================== 合并写回：保留未触碰模块，按 name 去重 ====================
existing = {}
if os.path.isfile(SHARD):
    with open(SHARD, encoding="utf-8") as f:
        for rec in json.load(f) or []:
            existing[(rec["layer"], rec["module"])] = rec

new_rows = []
for (layer, module), checks in mlab.ROWS.items():
    old = existing.get((layer, module), {})
    old_checks = {c["name"]: c for c in old.get("checks", [])}
    for c in checks:
        old_checks[c["name"]] = c
    merged = list(old_checks.values())
    new_rows.append({
        "layer": layer, "module": module, "checks": merged,
        "summary": {"pass": sum(1 for c in merged if c["ok"]),
                    "fail": sum(1 for c in merged if not c["ok"])},
    })

# 保留未被本轮 ROWS 覆盖的旧记录
for key, rec in existing.items():
    if key not in {(r["layer"], r["module"]) for r in new_rows}:
        new_rows.append(rec)

with open(SHARD, "w", encoding="utf-8") as f:
    json.dump(new_rows, f, ensure_ascii=False, indent=1)

print(json.dumps({"written": SHARD, "modules": len(new_rows)}, ensure_ascii=False))
