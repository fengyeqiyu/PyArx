import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
print("[N] numEntities=%s" % space.numEntities())
li = Db.Line(); li.setDatabaseDefaults()
li.setStartPoint(Ge.Point3d(0,30,0)); li.setEndPoint(Ge.Point3d(100,30,0))
print("[N] line ok, id=%s" % li.objectId())
try:
    print("[N] noop -> %r" % space._probeNoop(li))
except Exception as ex:
    print("[N] noop -> %s: %s" % (type(ex).__name__, str(ex)[:80]))
try:
    print("[N] noop on closed? -> %r" % space._probeNoop(li))
except Exception as ex:
    print("[N] noop2 -> %s: %s" % (type(ex).__name__, str(ex)[:80]))