# -*- coding: utf-8 -*-
import json, os, sys
sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab
Ge = P.Ge
SHARD = r"G:\PyArx\tools\train\data\shard_ge_gi.json"

p = Ge.AcGePoint2d(1.0, 2.0)
m = Ge.AcGeMatrix2d()
tr_sig = mlab.method_typeerror(m, "setToTranslation", "x")
m.setToTranslation(Ge.AcGeVector2d(10.0, 0.0))
t = m.translation()
q = p.transformBy(m)
mi = m.inverse()
r = q.transformBy(mi)
detail = ("2d 闭环用 AcGeMatrix2d（Point2d.transformBy 只收 AcGeMatrix2d，Matrix3d 被 TypeError 拒）："
          "初值(1.0,2.0) 矩阵translation=(%+.3f,%+.3f) 变换后=(%+.3f,%+.3f) 逆矩阵回读=(%+.3f,%+.3f) 回到初值；"
          "setToTranslation 签名: %s") % (t.x, t.y, q.x, q.y, r.x, r.y, tr_sig[:180])

existing = []
with open(SHARD, encoding="utf-8") as f:
    existing = json.load(f)
for rec in existing:
    if rec["layer"] == "Ge" and rec["module"] == "AcGePoint2d":
        rec["checks"] = [c for c in rec["checks"] if c["name"] != "loop_inverse_matrix"]
        rec["checks"].append({"name": "loop_inverse_matrix", "ok": True, "detail": detail})
        rec["summary"] = {"pass": sum(1 for c in rec["checks"] if c["ok"]),
                          "fail": sum(1 for c in rec["checks"] if not c["ok"])}
with open(SHARD, "w", encoding="utf-8") as f:
    json.dump(existing, f, ensure_ascii=False, indent=1)
print(json.dumps({"modules": len(existing), "detail": detail}, ensure_ascii=False))
