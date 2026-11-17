"""克隆/取图元副本的最小可用性验证（只碰全新对象，避免指针失效）。"""
import PyArxCore as C

Db, Ge = C.Db, C.Ge


def s(tag, fn):
    try:
        print("[E4] %-38s OK   %s" % (tag, str(fn())[:70]))
    except Exception as ex:
        print("[E4] %-38s FAIL %s: %s" % (tag, type(ex).__name__, str(ex)[:80]))


def fresh_line():
    return Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(2, 0, 0))


s("cloneFrom(fresh obj)", lambda: Db.Circle().cloneFrom(fresh_line()))
s("fresh.clone()", lambda: fresh_line().clone())
s("fresh.getTransformedCopy", lambda: fresh_line().getTransformedCopy(Ge.Matrix3d().setToTranslation(Ge.Vector3d(1, 0, 0))))
s("Db.DbObject(old_id)", lambda: Db.DbObject(1))
s("Circle(old_id, mode)", lambda: Db.Circle(1, 0))
s("Entity __init__ overloads", lambda: (Db.Entity.__init__.__doc__ or "").count("__init__("))
names = [n for n in dir(Db) if not n.startswith("_") and ("open" in n.lower() or "graph" in n.lower())]
print("[E4] Db open-ish names: %s" % ",".join(names))
