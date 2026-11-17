# -*- coding: utf-8 -*-
"""探针：块参照的 blockTransform 是否等于 R(rot)*S(sx,sy)，有没有剪切。"""
import json
import traceback
import PyArxCore as P

OUT = r"C:\temp\_probe_xf.json"
res = {"n": 0, "shear": 0, "samples": [], "err": ""}
try:
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        ms = tr.openObject(db.currentSpaceId(), 0)
        for eid in list(ms.entityIds()):
            o = tr.openObject(eid, 0)
            if o.dxfName() not in ("INSERT", "MINSERT"):
                continue
            res["n"] += 1
            try:
                tm = o.blockTransform().toNumpy()
            except Exception:
                continue
            try:
                sf = o.scaleFactors()
                sx, sy = float(sf[0]), float(sf[1])
            except Exception:
                sx = sy = 1.0
            try:
                rot = float(o.rotation())
            except Exception:
                rot = 0.0
            m00, m01, m10, m11 = float(tm[0][0]), float(tm[0][1]), float(tm[1][0]), float(tm[1][1])
            import math
            c, s = math.cos(rot), math.sin(rot)
            e00, e01, e10, e11 = sx * c, -sy * s, sx * s, sy * c
            dev = max(abs(m00 - e00), abs(m01 - e01), abs(m10 - e10), abs(m11 - e11))
            if dev > 1e-4 * max(1.0, abs(sx), abs(sy)):
                res["shear"] += 1
            if len(res["samples"]) < 10:
                res["samples"].append({"sx": sx, "sy": sy, "r": rot,
                                       "m": [m00, m01, m10, m11], "dev": dev,
                                       "pos": [float(tm[0][3]), float(tm[1][3])]})
except Exception:
    res["err"] = traceback.format_exc()[-300:]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("xf probe done")
