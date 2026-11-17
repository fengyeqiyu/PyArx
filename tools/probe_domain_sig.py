import PyArxCore as C
Db = C.Db
def doc(obj, name):
    f = getattr(obj, name, None)
    print("[SG] %-28s %s" % (name, ((f.__doc__ or "").splitlines() or ["<none>"])[0][:110] if f else "MISSING"))

D = Db.Database
for n in ("__init__", "addToModelspace", "addToCurrentspace", "getBlockTable", "readDwgFile", "dxfIn", "dxfOut", "saveAs", "save", "getFilename"):
    doc(D, n)
print("[SG] --- BlockTable/Record ---")
BT = Db.BlockTable
for n in ("get", "getBlockTableRecord", "getModelSpaceId", "getBlockTableRecordId"):
    doc(BT, n)
BTR = Db.BlockTableRecord
for n in ("appendAcDbEntity", "appendEntity", "entityIds", "getBlockReferenceIds"):
    doc(BTR, n)
print("[SG] --- 图元构造 ---")
for c, n in (("Line", "__init__"), ("Circle", "__init__"), ("Text", "__init__"), ("Polyline", "__init__"), ("Hatch", "__init__")):
    doc(getattr(Db, c), n)
print("[SG] --- 克隆/保存 ---")
doc(Db.DbObject, "clone"); doc(Db.DbObject, "cloneFrom"); doc(Db.Entity, "clone")
print("[SG] --- Ge ---")
for n in ("__init__", "distanceTo", "transformBy"):
    doc(C.Ge.Point3d, n)