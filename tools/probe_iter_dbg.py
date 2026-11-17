import PyArxCore as C
Db, Ge = C.Db, C.Ge
db = Db.workingDb()
def mk(y):
    li = Db.Line(); li.setDatabaseDefaults()
    li.setStartPoint(Ge.Point3d(0,y,0)); li.setEndPoint(Ge.Point3d(100,y,0)); return li
for y in (10,20,30): db.addToModelspace(mk(y))
space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
ids = space.entityIds()
print("[I] num=%d ids=%d" % (space.numEntities(), len(ids)))
w=[e.objectId() for e in space.iterEntities(1, True)]
r=[e.objectId() for e in space.iterEntities(0, True)]
print("[I] w=%d r=%d" % (len(w), len(r)))
miss = [i for i in ids if i not in w]
print("[I] missing=%s" % [hex(i) for i in miss])
for i in miss:
    try:
        e = space.getEntityById(i, 0); print("[I] direct open %s -> %r cls=%s" % (hex(i), e.objectId()!=0, e.isKindOf('AcDbEntity')))
    except Exception as ex: print("[I] direct open err %s" % str(ex)[:60])