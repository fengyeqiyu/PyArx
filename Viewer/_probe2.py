# -*- coding: utf-8 -*-
"""探针：随机取若干带属性的 INSERT，记录 ATTRIB 原始坐标/字高 与 该 INSERT 的 blockTransform，
判断 ATTRIB 坐标是否已经含实例变换（WCS），以及字高是否需要乘缩放。"""
import json
import PyArxCore as P

OUT = r"C:\temp\probe2.json"
res = {"rows": [], "err": []}
db = P.Db.curDb()

with P.Db.PyDbTrans() as tr:
    try:
        sp = tr.openObject(db.currentSpaceId(), 0)
        eids = sp.entityIds() or []
    except Exception as ex:
        eids = []
        res["err"].append("space:" + str(ex)[:120])

    for beid in eids:
        if len(res["rows"]) >= 12:
            break
        try:
            o = tr.openObject(beid, 0)
        except Exception:
            continue
        if not hasattr(o, "attributeIds"):
            continue
        try:
            aids = list(o.attributeIds() or [])
        except Exception:
            aids = []
        if not aids:
            continue
        row = {"n": 0, "tm": None, "pos": None, "attrs": []}
        try:
            sub = tr.openObject(o.blockTableRecordId(), 0)
            row["blk"] = str(sub.getName())[-26:]
        except Exception:
            row["blk"] = "?"
        try:
            tm = o.blockTransform().toNumpy()
            row["tm"] = [[round(float(tm[r][c]), 4) for c in range(3)] for r in range(2)]
        except Exception as ex:
            row["tm"] = "e:" + str(ex)[:40]
        try:
            pp = o.position()
            row["pos"] = [round(float(pp.x), 2), round(float(pp.y), 2)]
        except Exception as ex:
            row["pos"] = "e:" + str(ex)[:30]
        for aid in aids[:3]:
            try:
                av = o.openAttribute(aid, P.Enum.OpenMode.kForRead, False)
                p = av.position()
                a = {"t": str(av.textString())[:18],
                     "p": [round(float(p.x), 2), round(float(p.y), 2)],
                     "h": round(float(av.height()), 4)}
                row["attrs"].append(a)
            except Exception as ex:
                row["attrs"].append({"e": str(ex)[:40]})
        row["n"] = len(aids)
        res["rows"].append(row)

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False)
print("rows=%d err=%s" % (len(res["rows"]), res["err"][:1]))
