"""定向排查上一轮 FAIL 项：setDatabaseDefaults 签名 / Matrix3d 平移 / saveAs 实际产物。"""
import os

import PyArxCore as C

Db, Ge = C.Db, C.Ge
TMP = r"G:/PyArx/tmp_cap"
os.makedirs(TMP, exist_ok=True)

db = Db.workingDb()


def doc(label, obj):
    try:
        d = (obj.__doc__ or "").strip().splitlines()[0]
    except Exception:
        d = "<no doc>"
    print("[C2] %-34s %s" % (label, d[:150]))


doc("Db.Entity.setDatabaseDefaults", Db.Entity.setDatabaseDefaults)
doc("Db.Database.saveAs", Db.Database.saveAs)
doc("Db.Database.readDwgFile", Db.Database.readDwgFile)
doc("Ge.Matrix3d.translation", Ge.Matrix3d.translation)

li = Db.Line()
for how in ("noarg", "withdb"):
    try:
        if how == "noarg":
            li.setDatabaseDefaults()
        else:
            li.setDatabaseDefaults(db)
        print("[C2] setDatabaseDefaults(%s)=OK" % how)
        break
    except Exception as ex:
        print("[C2] setDatabaseDefaults(%s)=%s: %s" % (how, type(ex).__name__, str(ex)[:200]))

try:
    li.setStartPoint(Ge.Point3d(0, 0, 0))
    li.setEndPoint(Ge.Point3d(100, 0, 0))
    print("[C2] line.points OK", li.startPoint().x, li.endPoint().x)
except Exception as ex:
    print("[C2] line.points FAIL %s: %s" % (type(ex).__name__, str(ex)[:120]))

space = None
try:
    space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
    print("[C2] space opened OK")
except Exception as ex:
    print("[C2] space FAIL %s: %s" % (type(ex).__name__, str(ex)[:120]))

if space is not None:
    try:
        space.appendAcDbEntity(li)
        print("[C2] appendAcDbEntity OK id=", li.objectId().asOldId())
    except Exception as ex:
        print("[C2] append FAIL %s: %s" % (type(ex).__name__, str(ex)[:140]))

for m in ("translation", "setToTranslation", "setToIdentity"):
    if hasattr(Ge.Matrix3d, m):
        doc("Ge.Matrix3d." + m, getattr(Ge.Matrix3d, m))
print("[C2] LineSeg3d methods:", [n for n in dir(Ge.LineSeg3d) if not n.startswith("_")][:30])

out = os.path.join(TMP, "c2_out.dwg")
try:
    r = db.saveAs(out)
    print("[C2] saveAs returned %r exists=%s" % (r, os.path.exists(out)))
except Exception as ex:
    print("[C2] saveAs FAIL %s: %s" % (type(ex).__name__, str(ex)[:160]))

dxf = os.path.join(TMP, "c2_out.dxf")
try:
    r = db.dxfOut(dxf, 16)
    print("[C2] dxfOut returned %r exists=%s" % (r, os.path.exists(dxf)))
except Exception as ex:
    print("[C2] dxfOut FAIL %s: %s" % (type(ex).__name__, str(ex)[:160]))

print("[C2] tmp files:", sorted(os.listdir(TMP)))
