import json, PyArxCore as P

db = P.Db.curDb()
out = {}

def probe(obj, names):
    res = {}
    for n in names:
        try:
            f = getattr(obj, n)
            res[n] = str(f() if callable(f) else f)
        except Exception as e:
            res[n] = "ERR:" + type(e).__name__
    return res

with P.Db.PyDbTrans() as tr:
    ids = db.objectIds("AcDbLine")
    if ids:
        out["line"] = probe(tr.openObject(ids[0], 0),
                            ["startPoint", "endPoint", "length", "angle", "layer", "handle", "getGeomExtents"])
    mt = db.objectIds("AcDbMText")
    if mt:
        out["mtext"] = probe(tr.openObject(mt[0], 0),
                             ["contents", "textString", "location", "textHeight", "rotation", "layer", "handle"])
    br = db.objectIds("AcDbBlockReference")
    if br:
        b = tr.openObject(br[0], 0)
        out["blockref"] = probe(b, ["blockTableRecordId", "blockId", "position", "rotation", "scaleFactors", "layer", "handle"])
        bid = None
        try:
            bid = b.blockTableRecordId()
        except Exception:
            pass
        if bid:
            rec = tr.openObject(bid, 0)
            out["blockrec"] = probe(rec, ["name", "getName", "className"])

json.dump(out, open(r"G:\PyArx\log\tray_probe3.json", "w", encoding="utf-8"), ensure_ascii=False, default=str)
