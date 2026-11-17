import json

P = globals().get("P")
if P is None:
    import PyArxCore as P

log = {}
OUT = r"G:\PyArx\tools\cadmcp\api_probe3_out.json"


def rec(k, fn):
    try:
        log[k] = {"ok": True, "val": str(fn())[:300]}
    except Exception as e:
        log[k] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:400]}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


db = P.Db.curDb()

rec("ed_cmd_methods", lambda: [a for a in dir(P.Ed) if "cmd" in a.lower() or "Command" in a])


def bt_cast():
    with P.Db.PyDbTrans() as tr:
        raw = tr.openObject(db.blockTableId(), 1)
        t = type(raw).__name__
        casted = raw.cast()
        return {"raw": t, "cast": type(casted).__name__, "has_add": hasattr(casted, "add")}


rec("bt_cast", bt_cast)


def block_def2():
    with P.Db.PyDbTrans() as tr:
        bt = tr.openObject(db.blockTableId(), 1).cast()
        btr = P.Db.PyDbBlockTableRecord()
        btr.setName("PYX_B2")
        ok = bt.add(btr)
        tr.addNewlyCreatedDBObject(btr, True)
        c = P.Db.PyDbCircle(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGeVector3d(0, 0, 1), 20.0)
        btr.appendAcDbEntity(c)
        tr.addNewlyCreatedDBObject(c, True)
    return "ok add=" + str(ok)


rec("block_def2", block_def2)

log["done"] = True
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
