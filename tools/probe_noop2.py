import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
print("[N] before num=%s" % space.numEntities())
li = Db.Line(); li.setDatabaseDefaults()
li.setStartPoint(Ge.Point3d(0,30,0)); li.setEndPoint(Ge.Point3d(100,30,0))
try:
    r = space._probeAppend2(li)
    print("[N] probeAppend2 rc=%r" % r)
except Exception as ex:
    print("[N] probeAppend2 -> %s: %s" % (type(ex).__name__, str(ex)[:90]))
print("[N] after num=%s" % space.numEntities())
ids = space.entityIds()
print("[N] ids=%r" % (ids,))