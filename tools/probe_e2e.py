"""端到端：建库→加图元→关闭→DXF 出/入→DWG 存/读→克隆。逐步打印。"""
import os
import traceback

import PyArxCore as C

Db, Ge = C.Db, C.Ge
TMP = r"G:/PyArx/log/tmp"
if not os.path.isdir(TMP):
    os.makedirs(TMP)
DXF = TMP + "/e2e_out.dxf"
DWG = TMP + "/e2e_out.dwg"


def step(tag, fn):
    try:
        v = fn()
        print("[E2] %-36s OK   %s" % (tag, str(v)[:80]))
        return v
    except Exception as ex:
        print("[E2] %-36s FAIL %s: %s" % (tag, type(ex).__name__, str(ex)[:100]))
        return None


db = step("new Database(True,False)", lambda: Db.Database(True, False))
line = step("new Line", lambda: Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(7, 3, 0)))
oid = step("addToModelspace(line)", lambda: db.addToModelspace(line))
step("line.close()", lambda: line.close())
bt = step("getBlockTable(0)", lambda: db.getBlockTable(0))
print("[E2] bt type = %s" % type(bt).__name__)
ms = step("bt.get('*Model_Space',None,0)", lambda: bt.get("*Model_Space", None, 0))
print("[E2] ms type = %s has_objectId=%s" % (type(ms).__name__, hasattr(ms, "objectId")))
btr = step("BlockTableRecord(ms.objectId,0)", lambda: Db.BlockTableRecord(ms.objectId, 0))
if btr is None:
    btr = step("BlockTableRecord(int(oid),0)", lambda: Db.BlockTableRecord(int(oid), 0))
step("btr.entityIds len", lambda: len(btr.entityIds()) if btr else "no btr")
step("reopen line by id", lambda: Db.BlockTableRecord(btr.entityIds()[0], 0) if btr else "skip")

step("db.dxfOut", lambda: db.dxfOut(DXF))
step("dxf size", lambda: os.path.getsize(DXF) if os.path.exists(DXF) else "NONE")
d2 = step("new Database(False,False)", lambda: Db.Database(False, False))
step("d2.dxfIn", lambda: d2.dxfIn(DXF))
bt2 = step("d2.getBlockTable(0)", lambda: d2.getBlockTable(0))
ms2 = step("d2 ms record", lambda: bt2.get("*Model_Space", None, 0) if bt2 else None)
step("d2 btr.entityIds", lambda: len(Db.BlockTableRecord(ms2.objectId, 0).entityIds()) if ms2 else "skip")

step("db.saveAs", lambda: db.saveAs(DWG))
step("dwg size", lambda: os.path.getsize(DWG) if os.path.exists(DWG) else "NONE")
d3 = step("new Database(False,False) #2", lambda: Db.Database(False, False))
step("d3.readDwgFile(saved)", lambda: d3.readDwgFile(DWG) if os.path.exists(DWG) else "skip")
bt3 = step("d3.getBlockTable(0)", lambda: d3.getBlockTable(0))
ms3 = step("d3 ms record", lambda: bt3.get("*Model_Space", None, 0) if bt3 else None)
step("d3 btr.entityIds", lambda: len(Db.BlockTableRecord(ms3.objectId, 0).entityIds()) if ms3 else "skip")

step("circle ctor", lambda: Db.Circle(Ge.Point3d(1, 1, 0), Ge.Vector3d(0, 0, 1), 1.0))
ci = Db.Circle(Ge.Point3d(1, 1, 0), Ge.Vector3d(0, 0, 1), 1.0)
step("cloneFrom(line)", lambda: Db.Circle().cloneFrom(line))
step("line.clone()", lambda: line.clone())
step("getTransformedCopy", lambda: line.getTransformedCopy(Ge.Matrix3d().setToTranslation(Ge.Vector3d(2, 0, 0))))
