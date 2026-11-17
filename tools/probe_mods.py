# -*- coding: utf-8 -*-
import PyArxCore as C
print("[K] attrs=%s" % [n for n in dir(C) if not n.startswith('_')][:20])
for sub in ("Db", "Ge", "Ed", "Ap"):
    print("[K] %s=%r" % (sub, hasattr(C, sub)))
db = C.Db.workingDb()
sp = C.Db.BlockTableRecord(db.currentSpaceId(), C.Db.OpenMode.kForRead)
e = sp.getEntityById(sp.entityIds()[0], 0)
print("[K] className=%r" % e.className())
