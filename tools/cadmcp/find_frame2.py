# -*- coding: utf-8 -*-
"""找包含 (491900,-304600) 的大块参照 / 找出该处所有图层登记，判断图框归属"""
import json
import PyArxCore as P

TX, TY = 491900.0, -304600.0
db = P.Db.curDb()
out = {"target": [TX, TY], "blocks": [], "layers": {}, "err": []}


def bb(o):
    e = o.getGeomExtents()
    return (e[0].x, e[0].y, e[1].x, e[1].y)


with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbBlockReference"):
        try:
            o = tr.openObject(eid, 0)
            b = bb(o)
            w, h = b[2] - b[0], b[3] - b[1]
            if w < 5000 and h < 5000:
                continue
            ln = ""
            try:
                ln = str(o.layer())
            except Exception:
                pass
            inner = 0
            block_name = "?"
            try:
                br = tr.openObject(o.blockTableRecordId(), 0)
                block_name = str(br.getName())
                inner = len(br.entityIds())
            except Exception:
                pass
            rec = {"blk": block_name, "layer": ln, "w": round(w, 1), "h": round(h, 1),
                   "bb": [round(v, 1) for v in b], "n": inner,
                   "contains": bool(b[0] <= TX <= b[2] and b[1] <= TY <= b[3])}
            if rec["contains"] or (w > 30000 and h > 20000):
                out["blocks"].append(rec)
        except Exception as ex:
            pass

    # 目标点周围 20000 半径内的实体按图层统计
    for cls in ("AcDbLine", "AcDbPolyline", "AcDbText", "AcDbMText", "AcDbBlockReference", "AcDbCircle", "AcDbArc"):
        for eid in db.objectIds(cls):
            try:
                o = tr.openObject(eid, 0)
                b = bb(o)
                if not (TX - 20000 <= (b[0] + b[2]) / 2 <= TX + 20000 and TY - 20000 <= (b[1] + b[3]) / 2 <= TY + 20000):
                    continue
                ln = ""
                try:
                    ln = str(o.layer())
                except Exception:
                    pass
                k = "%s|%s" % (cls, ln)
                out["layers"][k] = out["layers"].get(k, 0) + 1
            except Exception:
                pass

open(r"C:\temp\frame2.json", "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, indent=1))
result = "ok"
