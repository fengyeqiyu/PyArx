import json

import PyArxCore as P

OUT = r"G:\PyArx\tools\cadmcp\layer_api_out.json"
res = {}
db = P.Db.curDb()

try:
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        src = lt.getAt("0")
        res["src_type"] = type(src).__name__
        lr = P.Db.PyDbLayerTableRecord(src)
        lr.setName("PYX_API")
        res["add_ret"] = str(lt.add(lr))
        tr.addNewlyCreatedDBObject(lr, True)
except Exception as e:
    res["err"] = repr(e)

try:
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        res["layers"] = list(lt.names())
        res["has_PYX_API"] = bool(lt.has("PYX_API"))
except Exception as e:
    res["check_err"] = repr(e)

json.dump(res, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
