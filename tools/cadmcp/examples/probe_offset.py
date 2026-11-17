# -*- coding: utf-8 -*-
"""探测 getOffsetCurves：返回类型、能否直接 addEntity 入库。"""

import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\cadmcp")

import PyArxCore as P

db = P.Db.curDb()
with P.Db.PyDbTrans() as tr:
    ln = P.Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(100, 0, 0))
    tr.addEntity(ln)
src_handle = str(ln.handle())

out = {"src_handle": src_handle, "probe": []}
with P.Db.PyDbTrans() as tr:
    target = None
    for eid in db.objectIds("AcDbEntity"):
        o = tr.openObject(eid, 0)
        if o is None:
            continue
        if str(o.handle()) == src_handle:
            target = o
            break
    if target is None:
        out["probe"].append({"error": "handle not found"})
    else:
        out["probe"].append({"cls": target.className(),
                             "has_offset": hasattr(target, "getOffsetCurves")})
        try:
            curves = target.getOffsetCurves(10.0)
            out["probe"].append({"ret_type": type(curves).__name__,
                                 "len": (len(curves) if hasattr(curves, "__len__") else "?")})
            added = 0
            for c in curves:
                kind = c.className() if hasattr(c, "className") else type(c).__name__
                try:
                    ok = tr.addEntity(c)
                    added += 1 if ok else 0
                    out["probe"].append({"offset_cls": kind, "add": bool(ok)})
                except Exception as ex:
                    out["probe"].append({"offset_cls": kind, "add_err": repr(ex)[:100]})
            out["added"] = added
        except Exception as ex:
            out["probe"].append({"offset_err": repr(ex)[:160]})

out["after_entities"] = [str(h) for h in db.objectIds("AcDbEntity")]
with open(r"C:\temp\offset_probe.json", "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=2)
print(json.dumps(out, ensure_ascii=False)[:800])
