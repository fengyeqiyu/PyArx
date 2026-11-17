# -*- coding: utf-8 -*-
import json, sys
sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab
Ge = P.Ge

out = {}
M2 = getattr(Ge, "AcGeMatrix2d", None)
out["m2_exists"] = M2 is not None
if M2 is not None:
    out["m2_members"] = [x for x in dir(M2) if not x.startswith("_")]
    out["m2_ctor"] = mlab.typeerror_hint(M2, "x")
    p = Ge.AcGePoint2d(1.0, 2.0)
    sig = mlab.method_typeerror(p, "transformBy", "x")
    out["p2_transformBy_sig"] = sig
    try:
        m = M2()
        for meth in ("setToTranslation", "setToScaling"):
            out[meth] = hasattr(m, meth)
        if hasattr(m, "setToTranslation"):
            m.setToTranslation(Ge.AcGeVector2d(10.0, 0.0))
        q = p.transformBy(m)
        mi = m.inverse()
        r = q.transformBy(mi)
        out["loop"] = "初值(1,2) 平移后(%+.3f,%+.3f) 回读(%+.3f,%+.3f)" % (q.x, q.y, r.x, r.y)
    except Exception as e:
        out["loop_err"] = "%s: %s" % (type(e).__name__, str(e)[:200])

with open(r"C:\temp\gegi_m2.json", "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1)
print(json.dumps(out, ensure_ascii=False)[:1500])
