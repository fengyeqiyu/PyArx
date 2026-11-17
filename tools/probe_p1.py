"""P1 新类型分步探查：逐步打印，定位崩溃点（无头 CAD 内执行）。"""
import sys

import PyArxCore as C

def step(msg):
    print("STEP " + msg)
    sys.stdout.flush()

step("import ok")
Db, Ed = C.Db, C.Ed
for n in ("Extents", "Extents2d", "ObjectIdArray", "IdPair"):
    step("%s=%s" % (n, hasattr(Db, n)))
step("Editor=%s Reactor=%s" % (hasattr(Ed, "Editor"), hasattr(Ed, "EditorReactor")))
step("acedCoordFromPixelToWorld=%s" % hasattr(Ed, "acedCoordFromPixelToWorld"))
step("Ge.Point3d=%s Ge.Point2d=%s" % (hasattr(C.Ge, "Point3d"), hasattr(C.Ge, "Point2d")))

e = Db.Extents()
step("Extents() ok isEmpty=%s" % e.isEmpty())
p = C.Ge.Point3d(1, 2, 3)
step("Point3d ok")
e.addPoint(p)
step("addPoint ok")
step("minPoint=%r" % (e.minPoint(),))
step("coords=%r" % (e.coords(),))
arr = Db.ObjectIdArray()
step("ObjectIdArray() ok len=%d" % len(arr))
arr.append(Db.ObjectId(3))
step("append ok len=%d" % len(arr))
step("Editor.className=%s" % Ed.Editor.className())
step("done")
