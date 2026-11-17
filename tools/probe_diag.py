import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
li = Db.Line(); li.setDatabaseDefaults()
li.setStartPoint(Ge.Point3d(0,0,0)); li.setEndPoint(Ge.Point3d(100,0,0))
try:
    r = space._diagAppend(li)
    print("[D] _diagAppend rc=%r" % r)
except Exception as ex:
    print("[D] _diagAppend raised %s" % type(ex).__name__)