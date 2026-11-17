import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
def mk(y):
    li = Db.Line(); li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0,y,0)); li.setEndPoint(Ge.Point3d(100,y,0)); return li
for name in ("_probeAppendMember", "appendEntity", "_diagAppend"):
    li = mk(30)
    try:
        r = getattr(space, name)(li)
        print("[P] %s -> %r id=%s" % (name, r, li.objectId()))
    except Exception as ex:
        print("[P] %s -> %s: %s" % (name, type(ex).__name__, str(ex)[:90]))
print("[P] numEntities=%s" % space.numEntities())