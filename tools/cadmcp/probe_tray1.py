import json, PyArxCore as P

out = {}
db = P.Db.curDb()
out["file"] = db.getFilename()
out["counts"] = {}
for cls in ("AcDbEntity", "AcDbPolyline", "AcDbLine", "AcDbBlockReference", "AcDbText", "AcDbMText", "AcDb2dPolyline"):
    try:
        out["counts"][cls] = len(db.objectIds(cls))
    except Exception as e:
        out["counts"][cls] = repr(e)

try:
    lt = db.getLayerTable()
    out["layers"] = [str(x) for x in lt.names()]
except Exception as e:
    out["layers"] = repr(e)

try:
    bt = db.getBlockTable()
    out["blocks"] = [str(x) for x in bt.names()]
except Exception as e:
    out["blocks"] = repr(e)

out["Ed_has"] = [n for n in ("ssget", "sslength", "ssname", "entget", "entnext", "handent") if hasattr(P.Ed, n)]

# 采样一个 polyline 和一个 block reference，看可用方法
try:
    with P.Db.PyDbTrans() as tr:
        ids = db.objectIds("AcDbPolyline")
        if ids:
            pl = tr.openObject(ids[0], 0)
            out["polyline_dir"] = [n for n in dir(pl) if not n.startswith("_")]
except Exception as e:
    out["polyline_dir"] = repr(e)

try:
    with P.Db.PyDbTrans() as tr:
        ids = db.objectIds("AcDbBlockReference")
        if ids:
            br = tr.openObject(ids[0], 0)
            out["blockref_dir"] = [n for n in dir(br) if not n.startswith("_")]
            out["blockref_name"] = str(br.name()) if hasattr(br, "name") else None
except Exception as e:
    out["blockref_dir"] = repr(e)

json.dump(out, open(r"G:\PyArx\log\tray_probe1.json", "w", encoding="utf-8"), ensure_ascii=False, default=str)
