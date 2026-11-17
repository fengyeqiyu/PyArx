# -*- coding: utf-8 -*-
"""verify：新的一支脚本里核对上一支脚本写入是否还在（命令结束后仍驻留）。"""
import datetime, json

import PyArxCore as P

LOG = r"G:\PyArx\log\pyarx_learn.log"
RES = r"G:\PyArx\log\pyarx_verify.json"
OUT = {"done": False}


def log(tag, msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write("[%s] %-6s %s\n" % (datetime.datetime.now().strftime("%H:%M:%S"), tag, msg))


def run():
    db = P.Db.curDb()
    out = {"entities": len(db.objectIds("AcDbEntity")), "by_class": {}}
    for cls in ("AcDbLine", "AcDbCircle", "AcDbArc", "AcDbText", "AcDbEllipse",
                "AcDbPolyline", "AcDbPoint", "AcDbSolid", "AcDbBlockReference"):
        out["by_class"][cls] = len(db.objectIds(cls))
    lt = db.getLayerTable(0)
    out["layers"] = list(lt.keys())
    lt.close()
    bt = db.getBlockTable(0)
    out["blocks"] = list(bt.keys())
    rec = bt.getAt("PYX_T14_BLOCK", 0)
    out["block_isNull"] = rec.isNull()
    if not rec.isNull():
        out["block_ents"] = len(rec.entityIds())
        rec.close()
    bt.close()
    log("VERIFY", json.dumps(out, ensure_ascii=False, default=str)[:600])
    return out


OUT["data"] = run()
OUT["done"] = True
with open(RES, "w", encoding="utf-8") as f:
    json.dump(OUT, f, ensure_ascii=False, indent=1, default=str)
print("[PyArx] verify done")
