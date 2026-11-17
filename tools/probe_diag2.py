import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)

def mk(y):
    li = Db.Line(); li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0,y,0)); li.setEndPoint(Ge.Point3d(100,y,0))
    return li

for name in ("appendEntity", "appendAcDbEntityId", "appendAcDbEntity"):
    li = mk(10)
    try:
        r = getattr(space, name)(li)
        print("[D2] %s -> %r id=%s" % (name, r, li.objectId()))
    except Exception as ex:
        print("[D2] %s -> %s: %s" % (name, type(ex).__name__, str(ex)[:80]))
print("[D2] numEntities=%s" % space.numEntities())
try:
    print("[D2] db.addToModelspace=%s" % db.addToModelspace(mk(20)))
except Exception as ex:
    print("[D2] addToModelspace -> %s: %s" % (type(ex).__name__, str(ex)[:80]))