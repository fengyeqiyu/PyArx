# -*- coding: utf-8 -*-
"""探针3：同一属性块的 ATTDEF 字高 与 ATTRIB height() 对比，判定字高是否被正确返回。"""
import json
import PyArxCore as P

OUT = r"C:\temp\probe3.json"
res = {"rows": [], "err": []}
db = P.Db.curDb()

def attdefs(tr, btr, limit=6):
    out = []
    try:
        eids = btr.entityIds() or []
    except Exception:
        return out
    for e in eids:
        try:
            o = tr.openObject(e, 0)
        except Exception:
            continue
        cn = type(o).__name__
        if "AttributeDefinition" not in cn:
            continue
        d = {"cn": cn}
        for k, fn in (("tag", "tagString"), ("h", "height"), ("p", "position"),
                      ("txt", "textString")):
            try:
                v = getattr(o, fn)()
                if k == "p":
                    d[k] = [round(float(v.x), 3), round(float(v.y), 3)]
                elif k in ("h",):
                    d[k] = round(float(v), 4)
                else:
                    d[k] = str(v)[:24]
            except Exception as ex:
                d[k] = "e:" + str(ex)[:24]
        out.append(d)
        if len(out) >= limit:
            break
    return out

with P.Db.PyDbTrans() as tr:
    try:
        sp = tr.openObject(db.currentSpaceId(), 0)
        eids = sp.entityIds() or []
    except Exception as ex:
        eids = []
        res["err"].append(str(ex)[:120])

    cnt = 0
    for beid in eids:
        if cnt >= 4:
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
            continue
        if not aids:
            continue
        row = {"blk": "?", "attrs": [], "defs": []}
        try:
            sub = tr.openObject(o.blockTableRecordId(), 0)
            row["blk"] = str(sub.getName())[-26:]
            row["defs"] = attdefs(tr, sub)
        except Exception as ex:
            row["err"] = str(ex)[:60]
        try:
            tm = o.blockTransform().toNumpy()
            row["sc"] = [round(float(tm[0][0]), 3), round(float(tm[0][1]), 3),
                         round(float(tm[1][0]), 3), round(float(tm[1][1]), 3)]
        except Exception as ex:
            row["sc"] = "e:" + str(ex)[:30]
        for aid in aids[:3]:
            try:
                av = o.openAttribute(aid, P.Enum.OpenMode.kForRead, False)
                p = av.position()
                row["attrs"].append({"t": str(av.textString())[:18],
                                     "h": round(float(av.height()), 5),
                                     "p": [round(float(p.x), 2), round(float(p.y), 2)]})
            except Exception as ex:
                row["attrs"].append({"e": str(ex)[:40]})
        res["rows"].append(row)
        cnt += 1

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False)
print("rows=%d" % len(res["rows"]))
