import PyArxCore as C

Db, Ge = C.Db, C.Ge
print("[E5] before clone")
try:
    obj = Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(2, 0, 0)).clone()
    print("[E5] clone OK %s" % (obj,))
except Exception as ex:
    print("[E5] clone FAIL %s: %s" % (type(ex).__name__, str(ex)[:80]))
print("[E5] after clone")
