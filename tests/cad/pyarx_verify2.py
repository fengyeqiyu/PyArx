# -*- coding: utf-8 -*-
"""verify2：下一支脚本核对实体留存（命令结束后）。"""
import datetime, json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_verify2.json"


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


db = P.Db.curDb()
out = {"entities": len(db.objectIds("AcDbEntity"))}
for cls in ("AcDbLine", "AcDbCircle", "AcDbBlockReference"):
    out[cls] = len(db.objectIds(cls))
ids = db.objectIds("AcDbCircle")
out["circles"] = []
for oid in ids:
    obj = db.openObject(oid, 0)
    if obj.isNull():
        out["circles"].append("NULL")
        continue
    c = P.Db.PyDbCircle.cast(obj)
    out["circles"].append({"r": c.radius(), "c": [c.center().x, c.center().y]})
    obj.close()
out["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1, default=str)
log("VERIFY2", json.dumps(out, ensure_ascii=False, default=str)[:500])
print("[PyArx] verify2 done")
