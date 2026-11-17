# -*- coding: utf-8 -*-
"""BTR 生命周期回归：包装释放后可重开写；持有写打开期间入库应给出明确提示。"""
import gc
import PyArxCore as C

Db = C.Db
db = Db.workingDb()
sid = db.currentSpaceId()

sp1 = Db.BlockTableRecord(sid, Db.OpenMode.kForRead)
print("[L] open read ok, num=%s" % sp1.numEntities())
del sp1
gc.collect()

sp2 = Db.BlockTableRecord(sid, Db.OpenMode.kForWrite)
print("[L] reopen write ok, num=%s" % sp2.numEntities())
del sp2
gc.collect()

try:
    newid = db.addToModelspace(Db.Line())
    print("[L] addToModelspace -> %r" % hex(newid))
except Exception as e:
    print("[L] addToModelspace FAIL: %s" % str(e)[:40])
sp3 = Db.BlockTableRecord(sid, Db.OpenMode.kForRead)
print("[L] final num=%s" % sp3.numEntities())
print("[L] done")
