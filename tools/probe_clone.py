# -*- coding: utf-8 -*-
"""clone 生命周期与入库回归（不长期持有空间写打开，符合 ARX 单次打开约束）。"""
import gc
import PyArxCore as C

Db = C.Db
db = Db.workingDb()


def num():
    return Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead).numEntities()


def first_id():
    return Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead).entityIds()[0]


def clone_of_first():
    sp = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForRead)
    return sp.getEntityById(sp.entityIds()[0], 0).clone()


print("[C] before num=%s" % num())
cp = clone_of_first()
print("[C] clone type=%s id=%r" % (type(cp).__name__, cp.objectId()))
print("[C] addToModelspace(clone) -> %r" % hex(db.addToModelspace(cp)))
print("[C] after num=%s" % num())
cp2 = clone_of_first()
del cp2
gc.collect()
print("[C] 未入库副本丢弃后 gc ok")
print("[C] all done")
