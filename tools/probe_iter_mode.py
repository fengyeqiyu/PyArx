import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
def mk(y):
    li = Db.Line(); li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0,y,0)); li.setEndPoint(Ge.Point3d(100,y,0)); return li
db.addToModelspace(mk(10))
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead)
ids = space.entityIds()
print("[H] num=%s ids=%d" % (space.numEntities(), len(ids)))
w = [e.objectId() for e in space.iterEntities(1, True)]
r = [e.objectId() for e in space.iterEntities(0, True)]
print("[H] write-iter=%d read-iter=%d" % (len(w), len(r)))
print("[H] missing in write: %s" % [hex(i) for i in set(ids) - set(w)])