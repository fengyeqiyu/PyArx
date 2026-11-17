# -*- coding: utf-8 -*-
"""最小化定位：写打开实体后 setColorIndex 的失败点。"""
import PyArxCore as C

Db = C.Db
db = Db.workingDb()
sid = db.currentSpaceId()

sp = Db.BlockTableRecord(sid, Db.OpenMode.kForWrite)
ids = sp.entityIds()
print("[P] ids=%s" % len(ids))
i = ids[0]
e = sp.getEntityById(i, 1)
print("[P] entity id match=%r type=%s" % (e.objectId() == i, type(e).__name__))
try:
    print("[P] upgradeOpen -> %r" % (e.upgradeOpen(),))
except Exception as ex:
    print("[P] upgradeOpen ERR %s" % type(ex).__name__)
try:
    print("[P] colorIndex=%s" % e.colorIndex())
except Exception as ex:
    print("[P] colorIndex ERR %s" % type(ex).__name__)
try:
    e.setColorIndex(3, False)
    print("[P] setColorIndex(3,False) OK -> %s" % e.colorIndex())
except Exception as ex:
    print("[P] setColorIndex ERR %s %s" % (type(ex).__name__, str(ex)[:40]))
print("[P] done")
