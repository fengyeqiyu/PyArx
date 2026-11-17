import json
import PyArxCore as P

db = P.Db.curDb()
info = {}
with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbBlockReference"):
        br = tr.openObject(eid, 0)
        if not str(br.layer()).startswith("E-CABL-TRAY"):
            continue
        info["br_has"] = [m for m in dir(br) if "xtent" in m or "geom" in m.lower()]
        try:
            ex = br.getGeomExtents()
            info["ex_type"] = type(ex).__name__
            info["ex_has"] = [m for m in dir(ex) if not m.startswith("_")]
            try:
                lo = ex.minPoint()
                hi = ex.maxPoint()
                info["lo"] = (lo.x, lo.y, lo.z)
                info["hi"] = (hi.x, hi.y, hi.z)
            except Exception as e:
                info["pt_err"] = repr(e)
        except Exception as e:
            info["ex_err"] = repr(e)
        break

json.dump(info, open(r"G:\PyArx\log\probe_ext.json", "w", encoding="utf-8"), default=str, indent=1)
print(json.dumps(info, default=str)[:600])
