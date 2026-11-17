import json

P = globals().get("P")
if P is None:
    import PyArxCore as P

log = {}
OUT = r"G:\PyArx\tools\cadmcp\api_probe2_out.json"


def rec(k, fn):
    try:
        log[k] = {"ok": True, "val": str(fn())[:300]}
    except Exception as e:
        log[k] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:400]}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


db = P.Db.curDb()

# 表 add 的真实签名（故意传错类型）
rec("blockTable_add_sig", lambda: db.getBlockTable().add("x"))
rec("layerTable_add_sig", lambda: db.getLayerTable().add("x"))


# 块定义：getBlockTable().add(record)
def block_def():
    with P.Db.PyDbTrans() as tr:
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_B1")
        bt = db.getBlockTable()
        r = bt.add(btr)
        tr.addNewlyCreatedDBObject(btr, True)
        c = P.Db.PyDbCircle(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGeVector3d(0, 0, 1), 20.0)
        btr.appendAcDbEntity(c)
        tr.addNewlyCreatedDBObject(c, True)
    return "block def ok add=" + str(r)


rec("block_def", block_def)


# 块参照
def block_ref():
    with P.Db.PyDbTrans() as tr:
        br = P.Db.PyDbBlockReference()
        br.setBlockTableRecord(db.getBlockTable().getAt("PYX_B1").objectId())
        br.setPosition(P.Ge.AcGePoint3d(600, 0, 0))
        tr.addEntity(br)
    return str(br.handle())


rec("block_ref", block_ref)


# 图层：先看能不能用 add 直接加记录
def layer_new():
    lr_ctor = getattr(P.Db.PyDbLayerTableRecord, "__init__", None)
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        try:
            lr = P.Db.PyDbLayerTableRecord()
        except Exception as e:
            return "ctor_fail: " + type(e).__name__ + " " + str(e)[:120]
        lr.setName("PYX_L1")
        lt.add(lr)
        tr.addNewlyCreatedDBObject(lr, True)
    return "layer ok"


rec("layer_new", layer_new)

log["done"] = True
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
