# -*- coding: utf-8 -*-
"""探针2：块定义里有没有 ATTDEF（属性定义），以及定义内实体类型分布。"""
import json
import traceback
import PyArxCore as P

OUT = r"C:\temp\_att_probe2.json"
res = {"defs": [], "attdef_defs": 0, "err": ""}
try:
    db = P.Db.curDb()
    with P.Db.PyDbTrans() as tr:
        btr_ids = db.objectIds("AcDbBlockTableRecord")
        for bid in btr_ids:
            btr = tr.openObject(bid, 0)
            try:
                name = str(btr.getName())
            except Exception:
                continue
            if name.startswith("*"):
                continue
            try:
                eids = list(btr.entityIds())
            except Exception:
                continue
            types = {}
            attdefs = []
            for eid in eids:
                try:
                    e = tr.openObject(eid, 0)
                    t = e.dxfName()
                except Exception:
                    continue
                types[t] = types.get(t, 0) + 1
                if t == "ATTDEF":
                    d = {}
                    for m in ("tag", "textString", "height", "rotation"):
                        try:
                            v = getattr(e, m)()
                            d[m] = float(v) if m in ("height", "rotation") else str(v)
                        except Exception:
                            pass
                    p = None
                    try:
                        p = e.position()
                        d["pos"] = [float(p.x), float(p.y)]
                    except Exception:
                        pass
                    attdefs.append(d)
            if attdefs:
                res["attdef_defs"] += 1
                if len(res["defs"]) < 20:
                    res["defs"].append({"n": name[:50], "types": types, "attdefs": attdefs[:4]})
            elif len(res["defs"]) < 20 and types.get("INSERT"):
                res["defs"].append({"n": name[:50], "types": types, "nested": True})
except Exception:
    res["err"] = traceback.format_exc()[-400:]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("probe2 written")
