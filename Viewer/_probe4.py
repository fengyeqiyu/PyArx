# -*- coding: utf-8 -*-
"""探针4：找到 AXISO 的实例，打印定义里的实体类型直方图、属性值、矩阵；
同时统计落在平面图区域的顶层 INSERT。"""
import json
import PyArxCore as P

OUT = r"C:\temp\probe4.json"
res = {"axis": [], "near": [], "err": []}
db = P.Db.curDb()


def typeh(tr, btr):
    h = {}
    try:
        ids = btr.entityIds() or []
    except Exception:
        return h, 0
    for e in ids:
        try:
            o = tr.openObject(e, 0)
            cn = type(o).__name__
        except Exception:
            cn = "?"
        h[cn] = h.get(cn, 0) + 1
    return h, len(ids)


def walk(tr, btr, depth, m, seen):
    if depth > 4:
        return
    try:
        eids = btr.entityIds() or []
    except Exception:
        return
    for beid in eids:
        try:
            o = tr.openObject(beid, 0)
        except Exception:
            continue
        if not hasattr(o, "blockTableRecordId"):
            continue
        try:
            sub = tr.openObject(o.blockTableRecordId(), 0)
            nm = str(sub.getName())
        except Exception:
            continue
        pos = None
        try:
            pp = o.position()
            pos = [round(float(pp.x), 1), round(float(pp.y), 1)]
        except Exception:
            pass
        if "AXISO" in nm.upper() and len(res["axis"]) < 6:
            th, n = typeh(tr, sub)
            row = {"nm": nm[-30:], "d": depth, "pos": pos, "ents": n, "types": th}
            try:
                aids = list(o.attributeIds() or [])
                row["aids"] = len(aids)
                vals = []
                for aid in aids[:3]:
                    av = o.openAttribute(aid, P.Enum.OpenMode.kForRead, False)
                    vals.append(str(av.textString())[:16])
                row["vals"] = vals
            except Exception as ex:
                row["avErr"] = str(ex)[:40]
            try:
                tm = o.blockTransform().toNumpy()
                row["tm"] = [[round(float(tm[r][c]), 3) for c in range(3)] for r in range(2)]
            except Exception as ex:
                row["tm"] = "e:" + str(ex)[:30]
            res["axis"].append(row)
        if pos and 340000 <= pos[0] <= 400000 and -330000 <= pos[1] <= -250000 \
                and len(res["near"]) < 10:
            res["near"].append({"nm": nm[-24:], "pos": pos, "d": depth})
        if nm not in seen:
            seen[nm] = 1
            walk(tr, sub, depth + 1, m, seen)


with P.Db.PyDbTrans() as tr:
    try:
        sp = tr.openObject(db.currentSpaceId(), 0)
        walk(tr, sp, 0, None, {})
    except Exception as ex:
        res["err"].append(str(ex)[:120])

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False)
print("axis=%d near=%d" % (len(res["axis"]), len(res["near"])))
