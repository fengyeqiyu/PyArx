# -*- coding: utf-8 -*-
"""verify4：下一支脚本核对 batch17 写入是否留存并可打开。"""
import json

import PyArxCore as P

RES = r"G:\PyArx\log\pyarx_verify4.json"
LOG = r"G:\PyArx\log\pyarx_learn.log"

db = P.Db.curDb()
out = {"entities": len(db.objectIds("AcDbEntity")), "by_class": {}, "open_check": []}
for cls in ("AcDbLine", "AcDbCircle", "AcDbPoint"):
    out["by_class"][cls] = len(db.objectIds(cls))
for oid in db.objectIds("AcDbEntity"):
    obj = db.openObject(oid, 0)
    if obj.isNull():
        out["open_check"].append({"id": oid, "open": False})
        continue
    h = str(obj.handle())
    obj.close()
    out["open_check"].append({"id": oid, "open": True, "handle": h})
out["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1, default=str)
with open(LOG, "a", encoding="utf-8") as f:
    f.write("[VERIFY4] %s\n" % json.dumps(out, ensure_ascii=False, default=str)[:500])
print("[PyArx] verify4 done")
