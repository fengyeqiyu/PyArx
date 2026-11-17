# -*- coding: utf-8 -*-
"""查模型空间与布局空间，判断三张图框在哪。"""
import json
import traceback

OUT = r"C:\temp\probe8.json"
res = {"err": "", "model": 0, "layouts": [], "ps_ids": 0, "top": []}
try:
    import PyArxCore as P
    db = P.Db.curDb()
    tr = P.Db.PyDbTrans()
    sp = tr.openObject(db.currentSpaceId(), 0)
    res["model"] = len(sp.entityIds() or [])
    names = [n for n in dir(db) if "ayout" in n or "pace" in n]
    res["layouts"].append(names)
    try:
        res["ps_ids"] = len(db.objectIds("AcDbLayout") or [])
    except Exception as ex:
        res["ps_ids"] = "ERR:" + str(ex)[:60]
    for beid in (sp.entityIds() or [])[:40]:
        o = tr.openObject(beid, 0)
        nm = type(o).__name__
        if nm == "PyDbBlockReference":
            try:
                nm = str(tr.openObject(o.blockTableRecordId(), 0).getName())
            except Exception:
                pass
        res["top"].append(nm)
except Exception:
    res["err"] = traceback.format_exc()[-900:]

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False)
