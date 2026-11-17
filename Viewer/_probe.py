# -*- coding: utf-8 -*-
"""递归（3 层）从模型空间找到 _AXISO 实例，逐环节报告 attributeIds / openAttribute / textString。"""
import json
import PyArxCore as P

OUT = r"C:\temp\probe.json"
res = {"chain": [], "hit": [], "err": []}
db = P.Db.curDb()


def walk(tr, btr, depth, tag, seen):
    eids = []
    try:
        eids = btr.entityIds() or []
    except Exception:
        return
    if tag and len(res["chain"]) < 40:
        res["chain"].append({"d": depth, "n": str(btr.getName())[-22:]})
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
        if "AXISO" in nm.upper():
            test(o, nm)
            if len(res["hit"]) >= 8:
                return
        if depth < 2 and nm not in seen and "AXISO" not in nm.upper():
            seen[nm] = 1
            walk(tr, sub, depth + 1, False, seen)


def test(o, nm):
    row = {"def": nm[-18:], "t": type(o).__name__}
    try:
        aids = list(o.attributeIds())
        row["ids"] = len(aids)
    except Exception as ex:
        row["ids"] = "e:" + str(ex)[:34]
        res["hit"].append(row)
        return
    if aids:
        try:
            av = o.openAttribute(aids[0], P.Enum.OpenMode.kForRead, False)
            row["av"] = type(av).__name__
            try:
                row["null"] = bool(av.isNull())
            except Exception as ex:
                row["null"] = "e:" + str(ex)[:22]
            try:
                row["txt"] = str(av.textString())[:16]
            except Exception as ex:
                row["txt"] = "e:" + str(ex)[:38]
            try:
                row["h"] = round(float(av.height()), 3)
            except Exception as ex:
                row["h"] = "e:" + str(ex)[:30]
            try:
                p = av.position()
                row["p"] = [round(float(p.x), 2), round(float(p.y), 2)]
            except Exception as ex:
                row["p"] = "e:" + str(ex)[:30]
        except Exception as ex:
            row["avErr"] = str(ex)[:60]
    res["hit"].append(row)


with P.Db.PyDbTrans() as tr:
    try:
        sp = tr.openObject(db.currentSpaceId(), 0)
        walk(tr, sp, 0, True, {})
    except Exception as ex:
        res["err"].append(str(ex)[:150])
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False)
print("chain=%d hit=%d" % (len(res["chain"]), len(res["hit"])))
