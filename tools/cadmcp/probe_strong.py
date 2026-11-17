import json, PyArxCore as P

db = P.Db.curDb()
out = {"file": db.getFilename(), "has_newRecord": hasattr(P.Db.PyDbLayerTableRecord, "newRecord"),
       "has_openObject": hasattr(db, "openObject"),
       "counts": {}, "layers": [], "blocks": [], "texts": []}

for cls in ("AcDbEntity", "AcDbLine", "AcDbPolyline", "AcDb2dPolyline", "AcDbBlockReference",
            "AcDbText", "AcDbMText", "AcDbHatch"):
    try:
        out["counts"][cls] = len(db.objectIds(cls))
    except Exception as e:
        out["counts"][cls] = repr(e)

lt = db.getLayerTable()
out["layers"] = [str(n) for n in lt.names()]
bt = db.getBlockTable()
out["blocks"] = [str(n) for n in bt.names()]

# 抓所有含“桥架”或“mm”的文字
with P.Db.PyDbTrans() as tr:
    for cls in ("AcDbMText", "AcDbText"):
        for eid in db.objectIds(cls):
            o = tr.openObject(eid, 0)
            try:
                s = o.contents() if cls == "AcDbMText" else o.textString()
            except Exception:
                continue
            s = str(s)
            if "桥架" in s or "mm" in s.lower():
                out["texts"].append({"cls": cls, "layer": str(o.layer()), "text": s[:120]})

json.dump(out, open(r"G:\PyArx\log\strong_probe.json", "w", encoding="utf-8"), ensure_ascii=False, default=str)
print("layers=%d blocks=%d texts=%d" % (len(out["layers"]), len(out["blocks"]), len(out["texts"])))
