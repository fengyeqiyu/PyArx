# -*- coding: utf-8 -*-
"""探针：块参照属性（ATTRIB）能不能拿到。全部逻辑写在模块级（PYRUNFILE 直接执行）。"""
import json
import traceback
import PyArxCore as P

OUT = r"C:\temp\_att_probe.json"
res = {"ents": 0, "blocks": 0, "samples": [], "total_att": 0, "err": "", "stage": "start"}
try:
    db = P.Db.curDb()
    res["stage"] = "db"
    with P.Db.PyDbTrans() as tr:
        res["stage"] = "tr"
        ms = tr.openObject(db.currentSpaceId(), 0)
        ids = list(ms.entityIds())
        res["ents"] = len(ids)
        res["stage"] = "ents"
        for eid in ids:
            o = tr.openObject(eid, 0)
            cls = o.dxfName()
            if cls not in ("INSERT", "MINSERT", "AcDbBlockReference", "AcDbMInsertBlock"):
                continue
            res["blocks"] += 1
            try:
                btr = tr.openObject(o.blockTableRecordId(), 0)
                name = str(btr.getName())
            except Exception:
                name = "?"
            if hasattr(o, "attributeIds"):
                try:
                    v = o.attributeIds()
                    got = [type(v).__name__, len(v) if v else 0,
                           [str(x) for x in list(v)[:2]]]
                    res["total_att"] += len(v) if v else 0
                except Exception as ex:
                    got = ["EXC", 0, str(ex)[:60]]
            else:
                got = ["no-api", 0, [m for m in dir(o) if "attr" in m.lower()][:8]]
            if len(res["samples"]) < 12:
                s = {"n": name[:40], "ids": got}
                if got[1]:
                    for aid in list(o.attributeIds())[:2]:
                        d = {"aid_type": type(aid).__name__, "aid": str(aid)}
                        try:
                            a = o.openAttribute(aid, 0, False)
                            d["dxf"] = a.dxfName()
                            d["tag"] = a.tag() if hasattr(a, "tag") else None
                            d["txt"] = a.textString() if hasattr(a, "textString") else None
                            d["h"] = float(a.height()) if hasattr(a, "height") else None
                            d["rot"] = float(a.rotation()) if hasattr(a, "rotation") else None
                            p = a.position() if hasattr(a, "position") else None
                            d["pos"] = [float(p.x), float(p.y)] if p else None
                        except Exception as ex:
                            d["openAttribute"] = "EXC:%s" % str(ex)[:80]
                        s.setdefault("att", []).append(d)
                res["samples"].append(s)
    res["stage"] = "done"
except Exception:
    res["err"] = traceback.format_exc()[-400:]
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)
print("att probe written")
