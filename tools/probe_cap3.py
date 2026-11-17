"""定位 appendAcDbEntity 的 "unknown exception"：分别试 appendEntity / appendAcDbEntityId / db.addToBlock。"""
import PyArxCore as C

Db, Ge = C.Db, C.Ge
db = Db.workingDb()


def doc(label, obj):
    try:
        print("[C3] %-30s %s" % (label, (obj.__doc__ or "").strip().splitlines()[0][:130]))
    except Exception:
        print("[C3] %-30s <no doc>" % label)


doc("space.appendEntity", Db.BlockTableRecord.appendEntity)
doc("space.appendAcDbEntityId", Db.BlockTableRecord.appendAcDbEntityId)
if hasattr(Db.Database, "addToBlock"):
    doc("db.addToBlock", Db.Database.addToBlock)

space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
print("[C3] space id=%d" % space.objectId())


def mkline(y):
    li = Db.Line()
    li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0, y, 0))
    li.setEndPoint(Ge.Point3d(100, y, 0))
    return li


for name in ("appendEntity", "appendAcDbEntityId", "appendAcDbEntity"):
    li = mkline(10)
    try:
        r = getattr(space, name)(li)
        print("[C3] %s -> %r  newId=%s" % (name, r, li.objectId()))
    except Exception as ex:
        print("[C3] %s -> %s: %s" % (name, type(ex).__name__, str(ex)[:160]))

if hasattr(Db.Database, "addToBlock"):
    li = mkline(20)
    try:
        r = db.addToBlock(li)
        print("[C3] addToBlock -> %r id=%s" % (r, li.objectId()))
    except Exception as ex:
        print("[C3] addToBlock -> %s: %s" % (type(ex).__name__, str(ex)[:160]))

print("[C3] space numEntities=%s" % (space.numEntities() if hasattr(space, "numEntities") else "n/a"))
