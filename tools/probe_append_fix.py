import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
def mk(y):
    li = Db.Line(); li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0,y,0)); li.setEndPoint(Ge.Point3d(100,y,0)); return li
print("[G] addToModelspace -> %r" % db.addToModelspace(mk(10)))
print("[G] addToCurrentspace -> %r" % db.addToCurrentspace(mk(20)))
print("[G] addToBlock(bad id) ->", end=' ')
try:
    db.addToBlock(0, mk(30))
except Exception as e:
    print("防呆 OK: %s" % str(e)[:40])
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
print("[G] num=%s" % space.numEntities())
print("[G] appendEntity -> %r, num=%s" % (space.appendEntity(mk(40)), space.numEntities()))
print("[G] appendAcDbEntityId -> %r" % space.appendAcDbEntityId(mk(50)))
print("[G] appendAcDbEntity -> %r" % space.appendAcDbEntity(mk(60)))
n = sum(1 for e in space)
ids = space.entityIds()
print("[G] iter=%d len=%d contains=%r attdef=%d" % (n, len(space), ids[0] in space, len(space.attdefData())))
print("[G] getEntityById -> %r" % (space.getEntityById(ids[0], 0).objectId() != 0))