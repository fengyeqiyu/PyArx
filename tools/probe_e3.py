"""收尾探测：追加后如何重开/遍历/克隆；选择集与 setVar 口径。"""
import PyArxCore as C

Db, Ge, Ed = C.Db, C.Ge, C.Ed


def step(tag, fn):
    try:
        v = fn()
        print("[E3] %-40s OK   %s" % (tag, str(v)[:80]))
        return v
    except Exception as ex:
        print("[E3] %-40s FAIL %s: %s" % (tag, type(ex).__name__, str(ex)[:100]))
        return None


db = Db.Database(True, False)
live = Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(5, 0, 0))
print("[E3] live line startPoint = %s" % (live.startPoint,))
oid = step("addToModelspace", lambda: db.addToModelspace(Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(9, 1, 0))))
step("重新打开 Line(int_id,0)", lambda: Db.Line(oid, 0))
step("重新打开 Entity(int_id,0)", lambda: Db.Entity(oid, 0))
step("Entity(Db.ObjectId(oid),0)", lambda: Db.Entity(Db.ObjectId(oid), 0))
step("ObjectId(oid).asOldId()", lambda: Db.ObjectId(oid).asOldId())

ms = step("bt.get('*Model_Space')", lambda: db.getBlockTable(0).get("*Model_Space"))
if ms is not None:
    print("[E3] ms type=%s" % type(ms).__name__)
    step("ms.objectId", lambda: ms.objectId)
    step("ms.entityIds?", lambda: len(ms.entityIds()) if hasattr(ms, "entityIds") else "no entityIds")
    step("BlockTableRecord(ms.objectId)", lambda: Db.BlockTableRecord(ms.objectId, 0))

btr = step("BlockTableRecord(oid_of_ms?)", lambda: Db.BlockTableRecord(oid, 0))
if btr is not None:
    step("btr.entityIds len", lambda: len(btr.entityIds()))
    ids = btr.entityIds() if hasattr(btr, "entityIds") else []
    print("[E3] ids[0] type=%s" % (type(ids[0]).__name__ if ids else "empty"))
    if ids:
        step("按 id 取实体 Db.Entity(ids[0])", lambda: Db.Entity(ids[0], 0))
        step("按 id 取实体 Db.Line(ids[0])", lambda: Db.Line(ids[0], 0))

step("cloneFrom(活对象)", lambda: Db.Circle().cloneFrom(Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(2, 0, 0))))
step("live.clone()", lambda: live.clone())
step("live.getTransformedCopy", lambda: live.getTransformedCopy(Ge.Matrix3d().setToTranslation(Ge.Vector3d(1, 0, 0))))

print("[E3] --- 交互 ---")
step("Ed.Core.ssget?", lambda: hasattr(Ed.Core, "ssget"))
step("Ed.Editor.ssget?", lambda: hasattr(Ed.Editor, "ssget"))
step("Ed.Core.setVar(TypedValue)", lambda: Ed.Core.setVar("CLAYER", Db.TypedValue(Db.DxfCode.kDxfTextValue, "0"))
     if False else Ed.Core.setVar("CLAYER", Db.TypedValue(8, "0")))
step("Ed.Editor.getPoint 存在", lambda: hasattr(Ed.Editor, "getPoint"))
step("Db.SelectionSet 存在", lambda: hasattr(Db, "SelectionSet"))
print("[E3] Ed.Core members = %s" % ",".join(sorted(x for x in dir(Ed.Core) if not x.startswith("_"))[:40]))
