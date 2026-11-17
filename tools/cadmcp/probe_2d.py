import json
import PyArxCore as P

db = P.Db.curDb()
info = {"layers": {}, "types": {}, "block_rec_has": [], "layer_rec_has": [], "block_rec_has": []}

lt = db.getLayerTable()
try:
    for lid in lt.objectIds():
        lr = lt.getAt(lid) if hasattr(lt, "getAt") else None
        info.setdefault("layer_rec_has", [])
        if lr is not None:
            info["layer_rec_has"] = [m for m in dir(lr) if not m.startswith("_")]
            break
except Exception as e:
    info["layer_err"] = repr(e)

# 层色/线型
try:
    for lid in lt.objectIds():
        lr = lt.getAt(lid)
        try:
            nm = str(lr.getName())
        except Exception:
            nm = "?"
        if not nm.startswith("E-CABL-TRAY"):
            continue
        d = {}
        for m in ("color", "lineWeight", "linetype", "isOff", "isFrozen", "isLocked"):
            if hasattr(lr, m):
                try:
                    d[m] = str(getattr(lr, m)())
                except Exception as e:
                    d[m] = "ERR:" + repr(e)
        try:
            c = lr.color()
            d["rgb"] = [str(x) for x in dir(c) if not x.startswith("_")]
            try:
                d["colorIndex"] = int(c.colorIndex())
                d["rgbValue"] = int(c.rgb())
            except Exception as e:
                d["c_err"] = repr(e)
        except Exception as e:
            d["c_err2"] = repr(e)
        info["layers"][nm] = d
except Exception as e:
    info["layer_scan_err"] = repr(e)

# 三个图层上的实体类型分布 + 块记录可用方法
cnt = {}
blocks = {}
try:
    with P.Db.PyDbTrans() as tr:
        for eid in db.objectIds("AcDbEntity"):
            o = tr.openObject(eid, 0)
            lay = str(o.layer())
            if not lay.startswith("E-CABL-TRAY"):
                continue
            t = type(o).__name__
            cnt[t] = cnt.get(t, 0) + 1
            if t == "PyDbBlockReference" or "BlockReference" in t:
                try:
                    btrid = o.blockTableRecordId()
                    rec = tr.openObject(btrid, 0)
                    nm = str(rec.getName())
                    blocks[nm] = blocks.get(nm, 0) + 1
                    if not info["block_rec_has"]:
                        info["block_rec_has"] = [m for m in dir(rec) if not m.startswith("_")]
                except Exception as e:
                    blocks["ERR"] = repr(e)
except Exception as e:
    info["ent_err"] = repr(e)

info["types"] = cnt
info["blocks"] = blocks
json.dump(info, open(r"G:\PyArx\log\probe_2d.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1, default=str)
print(json.dumps({"types": cnt, "layers": list(info["layers"])}, ensure_ascii=False))
