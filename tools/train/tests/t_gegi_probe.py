# -*- coding: utf-8 -*-
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

ge_targets = ["AcGePoint3d", "AcGePoint2d", "AcGeVector3d", "AcGeVector2d",
              "AcGeMatrix3d", "AcGeScale3d", "AcGeLineSeg3d", "AcGeCircArc3d",
              "AcGePlane", "AcGeCurve3d", "AcGeTol", "AcGeInterval"]
gi_targets = ["PyGiTextStyle", "PyGiDrawableTraits", "PyGiSubEntityTraits",
              "PyGiGeometry", "PyGiWorldGeometry", "PyGiDrawable", "PyGiWorldDraw",
              "PyGiCommonDraw", "PyGiViewportDraw", "PyGiViewportGeometry",
              "PyGiColor", "PyGiPolyline", "PyGiContextGdi"]

def inspect(mod, name):
    m = getattr(P, mod, None)
    if m is None:
        return {"exists": False, "why": "P.%s 子模块不存在" % mod}
    cls = getattr(m, name, None)
    if cls is None:
        return {"exists": False, "why": "P.%s 无 %s" % (mod, name)}
    out = {"exists": True, "hint": mlab.typeerror_hint(cls, "x")}
    try:
        out["members"] = [x for x in dir(cls) if not x.startswith("_")]
    except Exception as e:
        out["members_err"] = "%s: %s" % (type(e).__name__, e)
    # 尝试无参实例，再抓实例成员/常量
    try:
        inst = cls()
        out["ctor0_ok"] = True
        out["inst_type"] = "%s.%s" % (type(inst).__module__, type(inst).__name__)
        try:
            out["inst_members"] = [x for x in dir(inst) if not x.startswith("_")]
        except Exception as e:
            out["inst_members_err"] = str(e)
    except Exception as e:
        out["ctor0_ok"] = False
        out["ctor0_err"] = "%s: %s" % (type(e).__name__, str(e)[:200])
    return out

res = {
    "Ge": {n: inspect("Ge", n) for n in ge_targets},
    "Gi": {n: inspect("Gi", n) for n in gi_targets},
}
res["ge_dir"] = sorted(x for x in dir(P.Ge) if not x.startswith("_")) if hasattr(P, "Ge") else []
res["gi_dir"] = sorted(x for x in dir(P.Gi) if not x.startswith("_")) if hasattr(P, "Gi") else []

out = mlab.out_path(r"C:\temp\gegi_probe.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print(json.dumps({"written": out, "ge": len(res["ge_dir"]), "gi": len(res["gi_dir"])}))
