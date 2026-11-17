# -*- coding: utf-8 -*-
"""种子轮：① 抓取目标类的真实签名/成员；② 用 PyDbCircle 跑一遍四类断言做样板。

结果落两个文件：
  G:\PyArx\tools\train\tests\_seed_out.json   —— 签名表 + 断言原文，供后续子代理参考
  G:\PyArx\tools\train\data\module_matrix.json —— 与验收口径一致的 matrix 片段
"""

import json
import sys
import traceback

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

OUT = r"G:\PyArx\tools\train\tests\_seed_out.json"
MATRIX = r"G:\PyArx\tools\train\data\module_matrix.json"

CURATED = {
    "Db": ["PyDbDatabase", "PyDbEntity", "PyDbObject", "PyDbTrans", "PyDbBlockTableRecord",
           "PyDbLayerTableRecord", "PyDbPolyline", "PyDbMText", "PyDbHatch", "PyDbText",
           "PyDbCircle", "PyDbLine", "PyDbArc", "PyDbBlockReference", "PyDbTable",
           "PyDbCurve", "PyDbViewport", "PyDbLayerTable", "PyDbBlockTable", "PyDbSymbolTable"],
    "Ge": ["AcGePoint3d", "AcGePoint2d", "AcGeVector3d", "AcGeMatrix3d", "AcGeScale3d",
           "AcGeLineSeg3d", "AcGeCircArc3d", "AcGePlane", "AcGeCurve3d", "AcGeVector2d"],
    "Gi": ["PyGiDrawable", "PyGiWorldGeometry", "PyGiWorldDraw", "PyGiGeometry",
           "PyGiDrawableTraits", "PyGiSubEntityTraits", "PyGiColor", "PyGiPolyline",
           "PyGiContextGdi", "PyGiTextStyle"],
    "Ed": ["PyEdEnt", "PyEdFunc", "PyEdSel", "PyEdSelectionSets", "PyEdJig", "PyEdUtil",
           "PyEdUcsVp", "AdsName", "PyAdsErrorStatus", "PyAdsDxfCode", "PyAdsPromptStatus",
           "PyEdResult", "SelectionSet", "AcEdJig"],
    "Ap": ["PyApp", "Document", "DocumentManager", "PaletteSet", "Palette"],
}
# 无该子模块的组（Engine 侧类在 PyArxCore 里未必有模块，都按"存在性"如实记录）
GUESS_MODS = ["Engine", "Rx", "Core"]


def sig_of(mod, name):
    m = getattr(P, mod, None)
    if m is None:
        return {"exists": False, "why": "子模块 P.%s 不存在" % mod}
    obj = getattr(m, name, None)
    if obj is None:
        return {"exists": False, "why": "P.%s 里没有 %s" % (mod, name)}
    d = {}
    for probe in ("__init__", "__new__"):
        try:
            doc = getattr(obj, probe).__doc__
            if doc:
                d[probe] = doc.strip().splitlines()[0][:400]
        except Exception:
            pass
    try:
        if not d and obj.__doc__:
            d["__doc__"] = obj.__doc__.strip().splitlines()[0][:400]
    except Exception:
        pass
    try:
        mem = [x for x in dir(obj) if not x.startswith("_")]
    except Exception:
        mem = []
    return {"exists": True, "sig": d, "members": mem[:45], "member_count": len(mem)}


def collect_sigs():
    sigs = {}
    for mod, names in CURATED.items():
        sigs[mod] = {n: sig_of(mod, n) for n in names}
    for mod in GUESS_MODS:
        sigs[mod] = {"__module__": {"exists": hasattr(P, mod)}}
    return sigs


# ---------------------------------------------------------------- PyDbCircle 样板
L, M = "Db", "PyDbCircle"
C = P.Db.PyDbCircle
PT = P.Ge.AcGePoint3d(0.0, 0.0, 0.0)

FORMS = [(), (PT, 12.5), (P.Ge.AcGePoint2d(0.0, 0.0), 12.5), ((0.0, 0.0), 12.5)]


def make():
    inst, form = mlab.ctor(C, *FORMS)
    return inst, form


def t_exists_ctor():
    assert hasattr(P.Db, "PyDbCircle"), "P.Db 无 PyDbCircle"
    inst, form = make()
    return "构造 form=%s type=%s.%s members=%d" % (
        form, type(inst).__module__, type(inst).__name__, len([x for x in dir(inst) if not x.startswith("_")]))


def t_type_name():
    inst, _ = make()
    return "%s.%s" % (type(inst).__module__, type(inst).__name__)


def t_prop_roundtrip():
    inst, _ = make()
    inst.setRadius(33.25)
    r = inst.radius()
    assert abs(float(r) - 33.25) < 1e-9, "radius 读回不一致: %r" % (r,)
    inst.setCenter(P.Ge.AcGePoint3d(10.0, 20.0, 0.0))
    c = inst.center()
    if hasattr(c, "x"):
        x, y, z = c.x, c.y, c.z
    else:
        x, y, z = c[0], c[1], c[2]
    assert (abs(x - 10.0) < 1e-9 and abs(y - 20.0) < 1e-9), "center 读回不一致: %r" % (c,)
    return "radius 33.25->%.4f center->(%.3f,%.3f,%.3f) 回读类型=%s" % (r, x, y, z, type(c).__name__)


def t_db_roundtrip():
    db = P.Db.curDb()
    before = len(list(db.objectIds("AcDbEntity")))
    inst, form = make()
    inst.setRadius(7.5)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(inst)
        h = str(inst.handle())
        rr = float(inst.radius())
    after = len(list(db.objectIds("AcDbEntity")))
    assert h != "0", "句柄为 0，未真正入库"
    assert after > before, "commit 后实体数没增长: %d -> %d" % (before, after)
    return "form=%s before=%d after=%d handle=%s 事务内radius=%.3f" % (form, before, after, h, rr)


def t_readback():
    """commit 后按句柄再读回：试几种就记哪种可用。"""
    inst, _ = make()
    inst.setRadius(4.25)
    with P.Db.PyDbTrans() as tr:
        tr.addEntity(inst)
        h = str(inst.handle())
    tried = []
    db = P.Db.curDb()
    for api in ("getObjectIdByHandle", "objectIdByHandle"):
        fn = getattr(db, api, None)
        if fn is None:
            tried.append(api + ":missing")
            continue
        try:
            oid = fn(h)
            tried.append(api + ":" + str(oid))
            return "handle=%s 用 %s 读回 objectId=%s" % (h, api, oid)
        except Exception as e:
            tried.append("%s:%s" % (api, type(e).__name__))
    return "handle=%s 无可直接读回的句柄 API，尝试=%s" % (h, tried)


mlab.check(L, M, "exists+construct", t_exists_ctor)
mlab.check(L, M, "type_name", t_type_name)
mlab.check(L, M, "prop_roundtrip_radius_center", t_prop_roundtrip)
mlab.check(L, M, "db_roundtrip", t_db_roundtrip)
mlab.check(L, M, "readback_after_commit", t_readback)

# 失败面：期望是"明确失败"，静默接受要标 fail
mlab.probe(L, M, "ctor 无参", lambda: C(), allow=("Exception",))
mlab.probe(L, M, "ctor 负半径", lambda: C(PT, -5.0), allow=("Exception",))
mlab.probe(L, M, "ctor 半径传字符串", lambda: C(PT, "abc"), allow=("Exception",))
mlab.probe(L, M, "setRadius(0)", lambda: (lambda i: (i.setRadius(0.0), i.radius()))(C(PT, 1.0)),
           allow=("Exception",))


def main():
    rep = {"pyd": str(getattr(P, "__file__", "?")),
           "has_mods": {m: hasattr(P, m) for m in list(CURATED) + GUESS_MODS}}
    try:
        rep["sigs"] = collect_sigs()
    except Exception:
        rep["sigs_error"] = traceback.format_exc()
    rep["circle"] = mlab.report()
    rep["dump"] = mlab.dump(MATRIX)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    return {"written": OUT, "matrix": rep["dump"], "circle_summary": rep["circle"][0]["summary"] if rep["circle"] else None}


try:
    print(json.dumps(main(), ensure_ascii=False))
except Exception:
    print(json.dumps({"fatal": traceback.format_exc()}, ensure_ascii=False))
