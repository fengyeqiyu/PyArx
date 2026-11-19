import json
OUT = r"G:\PyArx\tools\cadmcp\dbg_guard.json"
P = globals().get("P")
if P is None:
    import PyArxCore as P
log = {}
def flush():
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)
db = P.Db.curDb()
tr = P.Db.PyDbTrans()
bt = tr.openObject(db.blockTableId(), 1)
btr = P.Db.PyDbBlockTableRecord()
btr.setName("GUARD_T")
log["add"] = str(bt.add(btr))
try:
    log["we_after_add"] = str(btr.isWriteEnabled())
except Exception as e:
    log["we_after_add"] = "ERR " + str(e)
flush()
try:
    log["post"] = str(tr.addNewlyCreatedDBObject(btr, True))
except Exception as e:
    log["post"] = "ERR " + str(e)
flush()
try:
    log["we_after_post"] = str(btr.isWriteEnabled())
except Exception as e:
    log["we_after_post"] = "ERR " + str(e)
flush()
c = P.Db.PyDbCircle(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGeVector3d(0, 0, 1), 5.0)
try:
    log["append"] = str(btr.appendAcDbEntity(c))
except Exception as e:
    log["append"] = "ERR " + type(e).__name__ + " " + str(e)[:200]
flush()
try:
    tr.abort()
except Exception:
    pass
log["done"] = True
flush()
