# -*- coding: utf-8 -*-
"""在当前文档里用 PyArx 画带图层的图元，然后 db.saveAs 落盘。"""

import os

import PyArxCore as P

PATH = r"C:\temp\pyarx_save_verify.dwg"

DB = P.Db.curDb()
lt = DB.getLayerTable()
if not lt.has("PYARX_SAVE"):
    rec = P.Db.PyDbLayerTableRecord.newRecord()
    rec.setName("PYARX_SAVE")
    rec.setColor(0, 150, 255)
    lt.add(rec)

with P.Db.PyDbTrans() as tr:
    ln = P.Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(120, 0, 0))
    tr.addEntity(ln)
    ln.setLayerId(lt.getAt("PYARX_SAVE").objectId())
    tx = P.Db.PyDbText(P.Ge.AcGePoint3d(0, 20, 0), "SAVE CYCLE", 8.0, 0.0)
    tr.addEntity(tx)
    tx.setLayerId(lt.getAt("PYARX_SAVE").objectId())

handles = [str(ln.handle()), str(tx.handle())]

doc = P.Ap.PyApp.documentManager.curDocument
st = str(doc.database.saveAs(PATH))

result = {
    "path": PATH,
    "status": st,
    "exists": os.path.exists(PATH),
    "size": os.path.getsize(PATH) if os.path.exists(PATH) else 0,
    "entities": len(DB.objectIds("AcDbEntity")),
    "handles": handles,
}
