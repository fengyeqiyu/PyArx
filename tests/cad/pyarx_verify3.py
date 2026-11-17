# -*- coding: utf-8 -*-
"""verify3：逐个 ObjectId 试开，判断 addToCurrentspace 留下的是"僵尸"还是"不存在"。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\pyarx_verify3.json"
LOG = r"G:\PyArx\log\pyarx_learn.log"

db = P.Db.curDb()
out = {"ids": []}
for oid in db.objectIds("AcDbEntity"):
    rec = {"id": oid}
    try:
        obj = db.openObject(oid, 0)
        rec["isNull"] = obj.isNull()
        if not obj.isNull():
            rec["handle"] = str(obj.handle())
            rec["erased"] = obj.isErased()
            obj.close()
    except Exception as e:
        rec["err"] = type(e).__name__ + ": " + str(e)[:100]
    out["ids"].append(rec)
out["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1, default=str)
with open(LOG, "a", encoding="utf-8") as f:
    f.write("[VERIFY3] %s\n" % json.dumps(out, ensure_ascii=False, default=str)[:400])
print("[PyArx] verify3 done")
