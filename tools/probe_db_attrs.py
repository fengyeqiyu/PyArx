# -*- coding: utf-8 -*-
import PyArxCore as C
db = C.Db.workingDb()
names = ["angdir","celweight","chamferc","chamferd","cmljust","cmlscale","dimfit","dimunit","dimassoc","blockTable","addObject"]
for n in names:
    print("[K] has", n, hasattr(db, n))
print("[K] blockTable type", type(db.blockTable(0)).__name__)
print("[K] sysvars", db.angdir(), db.celweight(), db.chamferc(), db.chamferd(), db.cmljust(), db.cmlscale(), db.dimfit(), db.dimunit(), db.dimassoc())
print("[K] probe db done")