# -*- coding: utf-8 -*-
"""在图里寻找包含 (491900,-304600) 的图框矩形（不看图层，看几何）"""
import json
import PyArxCore as P

TX, TY = 491900.0, -304600.0
db = P.Db.curDb()
out = {"target": [TX, TY], "cands": [], "big_lines": [], "err": []}


def bb(o):
    e = o.getGeomExtents()
    return (e[0].x, e[0].y, e[1].x, e[1].y)


def pts_of(o):
    r = []
    try:
        n = o.numVerts()
    except Exception:
        return r
    for i in range(n):
        try:
            q = o.getPointAt(i)
            r.append((round(q.x, 1), round(q.y, 1)))
        except Exception:
            pass
    return r


with P.Db.PyDbTrans() as tr:
    for cls in ("AcDbPolyline", "AcDb2dPolyline"):
        for eid in db.objectIds(cls):
            try:
                o = tr.openObject(eid, 0)
                b = bb(o)
                w, h = b[2] - b[0], b[3] - b[1]
                if b[0] <= TX <= b[2] and b[1] <= TY <= b[3] and w > 15000 and h > 8000:
                    ln = ""
                    try:
                        ln = str(o.layer())
                    except Exception:
                        pass
                    closed = False
                    try:
                        closed = bool(o.isClosed())
                    except Exception:
                        pass
                    out["cands"].append({"cls": cls, "layer": ln, "closed": closed,
                                         "bb": [round(v, 1) for v in b],
                                         "w": round(w, 1), "h": round(h, 1),
                                         "pts": pts_of(o)[:8], "nv": len(pts_of(o))})
            except Exception:
                pass
    # 大尺寸线（图框可能由 4 条 line 拼成）
    for eid in db.objectIds("AcDbLine"):
        try:
            o = tr.openObject(eid, 0)
            b = bb(o)
            w, h = b[2] - b[0], b[3] - b[1]
            if max(w, h) > 15000 and b[0] <= TX <= b[2] and b[1] <= TY <= b[3]:
                ln = ""
                try:
                    ln = str(o.layer())
                except Exception:
                    pass
                out["big_lines"].append({"layer": ln, "bb": [round(v, 1) for v in b]})
        except Exception:
            pass

open(r"C:\temp\frame.json", "w", encoding="utf-8").write(json.dumps(out, ensure_ascii=False, indent=1))
result = "ok"
