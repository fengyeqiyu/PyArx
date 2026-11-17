# -*- coding: utf-8 -*-
"""探针：blockTransform().toNumpy() 的行列布局 + 与 position/rotation/scale 的一致性。"""
import json
import math
import traceback
import PyArxCore as P

OUT = r"C:\temp\_probe_xf2.json"
res = {"samples": [], "err": ""}
try:
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        ms = tr.openObject(db.currentSpaceId(), 0)
        for eid in list(ms.entityIds()):
            o = tr.openObject(eid, 0)
            if o.dxfName() not in ("INSERT", "MINSERT"):
                continue
            rot = float(o.rotation())
            sf = o.scaleFactors()
            sx, sy = float(sf[0]), float(sf[1])
            if abs(rot) < 1e-6 or len(res["samples"]) >= 8:
                continue
            tm = o.blockTransform().toNumpy()
            pp = o.position()
            res["samples"].append({
                "rot_deg": round(math.degrees(rot), 3),
                "sx": sx, "sy": sy,
                "pos": [float(pp.x), float(pp.y)],
                "m00": float(tm[0][0]), "m01": float(tm[0][1]),
                "m10": float(tm[1][0]), "m11": float(tm[1][1]),
                "m03": float(tm[0][3]), "m13": float(tm[1][3]),
                "m30": float(tm[3][0]), "m31": float(tm[3][1]),
                "expect": [round(sx * math.cos(rot), 6), round(-sy * math.sin(rot), 6),
                           round(sx * math.sin(rot), 6), round(sy * math.cos(rot), 6)],
            })
except Exception:
    res["err"] = traceback.format_exc()[-300:]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("xf2 probe done")
