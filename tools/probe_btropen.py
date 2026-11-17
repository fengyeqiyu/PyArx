# -*- coding: utf-8 -*-
"""判定：显式 close 是否能让底层真正释放（对照 del 自动析构）。"""
import gc
import PyArxCore as C

Db = C.Db
db = Db.workingDb()
sid = db.currentSpaceId()

a = Db.BlockTableRecord(sid, Db.OpenMode.kForRead)
print("[N] a num=%s" % a.numEntities())
r = a.close()
print("[N] explicit close -> %r" % (r,))
del a
gc.collect()
b = Db.BlockTableRecord(sid, Db.OpenMode.kForWrite)
print("[N] after explicit close: b null=%r num=%s" % (b.isNull(), b.numEntities() if not b.isNull() else -1))
del b
gc.collect()

c1 = Db.BlockTableRecord(sid, Db.OpenMode.kForRead)
print("[N] c1 num=%s" % c1.numEntities())
del c1
gc.collect()
c2 = Db.BlockTableRecord(sid, Db.OpenMode.kForWrite)
print("[N] after del only: c2 null=%r num=%s" % (c2.isNull(), c2.numEntities() if not c2.isNull() else -1))
print("[N] done")
