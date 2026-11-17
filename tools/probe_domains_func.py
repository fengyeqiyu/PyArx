"""功能域实测：图元 / 几何 / 读写 / 保存 / 克隆 / 交互 / 打印 的真实可用性。

每步独立 try/except，结果以 [FT] 前缀打印，便于 grep。
"""
import os

import PyArxCore as C

Db, Ge, Ed, Ap = C.Db, C.Ge, C.Ed, C.Ap
TMP = r"G:/PyArx/log/tmp"
if not os.path.isdir(TMP):
    os.makedirs(TMP)

results = []


def test(tag, fn):
    try:
        val = fn()
        print("[FT] %-34s OK   %s" % (tag, "" if val is None else str(val)[:60]))
        results.append((tag, True))
        return val
    except Exception as ex:
        print("[FT] %-34s FAIL %s: %s" % (tag, type(ex).__name__, str(ex)[:90]))
        results.append((tag, False))
        return None


# ---------------------------------------------------------------- 图元
db = test("db.new", lambda: Db.Database(True, False))
line = test("entity.Line(2pt)", lambda: Db.Line(Ge.Point3d(0, 0, 0), Ge.Point3d(10, 0, 0)))
circle = test("entity.Circle(center,nrm,r)", lambda: Db.Circle(Ge.Point3d(5, 5, 0), Ge.Vector3d(0, 0, 1), 2.0))
text = test("entity.Text(pos,txt,h)", lambda: Db.Text(Ge.Point3d(0, 5, 0), "PYARX", 2.0, 0.0))
pl = test("entity.Polyline()", lambda: Db.Polyline())
test("entity.Arc(center,r,a0,a1)", lambda: Db.Arc(Ge.Point3d(0, 0, 0), 3.0, 0.0, 1.57))
test("append line -> modelspace", lambda: db.addToModelspace(line))
test("append circle -> modelspace", lambda: db.addToModelspace(circle))
test("append text -> modelspace", lambda: db.addToModelspace(text))
test("line.startPoint", lambda: line.startPoint)
test("line.endPoint", lambda: line.endPoint)
test("circle.radius", lambda: circle.radius)
test("text.textString", lambda: text.textString)
ms = test("db.getBlockTable().get(*Model_Space)", lambda: db.getBlockTable().get("*Model_Space"))
test("modelspace.entityIds()", lambda: len(ms.entityIds()))
test("entity.getGeomExtents()", lambda: line.getGeomExtents())
test("entity.layer", lambda: line.layer)
test("entity.erase()", lambda: circle.erase())

# ---------------------------------------------------------------- 几何
p1, p2 = Ge.Point3d(0, 0, 0), Ge.Point3d(3, 4, 0)
test("Ge.Point3d.distanceTo", lambda: p1.distanceTo(p2))
test("Ge.Vector3d.length", lambda: Ge.Vector3d(3, 4, 0).length())
test("Ge.Vector3d.normalize", lambda: Ge.Vector3d(3, 4, 0).normalize())
test("Ge.Vector3d.crossProduct", lambda: Ge.Vector3d(1, 0, 0).crossProduct(Ge.Vector3d(0, 1, 0)))
test("Ge.Vector3d.dotProduct", lambda: Ge.Vector3d(1, 0, 0).dotProduct(Ge.Vector3d(0, 1, 0)))
test("Ge.Matrix3d.setToTranslation", lambda: Ge.Matrix3d().setToTranslation(Ge.Vector3d(1, 2, 3)))
test("Ge.Point3d.transformBy", lambda: p1.transformBy(Ge.Matrix3d().setToTranslation(Ge.Vector3d(5, 0, 0))))
test("Ge.LineSeg3d.length", lambda: Ge.LineSeg3d(Ge.Point3d(0, 0, 0), Ge.Point3d(3, 4, 0)).length())
test("Ge.Plane.normal", lambda: Ge.Plane(Ge.Point3d(0, 0, 0), Ge.Vector3d(0, 0, 1)).normal())
test("Ge.Interval.length", lambda: Ge.Interval(0.0, 5.0).length())
test("Ge.NurbCurve3d?", lambda: hasattr(Ge, "NurbCurve3d"))

# ---------------------------------------------------------------- 交互（只做不阻塞的）
test("Ed.Core.getVar(CLAYER)", lambda: Ed.Core.getVar("CLAYER") if hasattr(Ed.Core, "getVar") else "no getVar")
test("Ed.Core.setVar(CLAYER)", lambda: Ed.Core.setVar("CLAYER", "0") if hasattr(Ed.Core, "setVar") else "no setVar")
test("Ed.Editor.getPoint?", lambda: hasattr(Ed.Editor, "getPoint"))
test("Ed.Editor.getInteger/getReal/getString", lambda: all(hasattr(Ed.Editor, n) for n in ("getInteger", "getReal", "getString")))
test("Ed.Editor.getCurrentSelectionSet", lambda: Ed.Editor.getCurrentSelectionSet())
test("Ed.Jig exists", lambda: hasattr(Ed, "Jig"))
test("Db.SelectionSet 类", lambda: hasattr(Db, "SelectionSet"))

# ---------------------------------------------------------------- 读写
dxf = TMP + "/pyarx_dom.dxf"
test("db.dxfOut(dxf)", lambda: db.dxfOut(dxf))
test("dxf 文件落盘", lambda: os.path.getsize(dxf))
db2 = test("db2.new(False)", lambda: Db.Database(False, False))
test("db2.dxfIn(dxf)", lambda: db2.dxfIn(dxf))
test("db2 modelspace 实体数", lambda: len(db2.getBlockTable().get("*Model_Space").entityIds()))
dwg_src = r"G:\PyArx\ACAD\Express\brkline.dwg"
db3 = test("db3.new(False)", lambda: Db.Database(False, False))
test("db3.readDwgFile(brkline)", lambda: db3.readDwgFile(dwg_src) if os.path.exists(dwg_src) else "no sample")
test("db3 实体数", lambda: len(db3.getBlockTable().get("*Model_Space").entityIds()))
test("db3.getFilename", lambda: db3.getFilename())

# ---------------------------------------------------------------- 保存
saved = TMP + "/pyarx_dom.dwg"
test("db.saveAs(dwg)", lambda: db.saveAs(saved))
test("dwg 文件落盘", lambda: os.path.getsize(saved) if os.path.exists(saved) else "NOT SAVED")
db4 = test("db4.new(False)", lambda: Db.Database(False, False))
test("db4.readDwgFile(刚保存的)", lambda: db4.readDwgFile(saved) if os.path.exists(saved) else "skip")
test("db4 实体数", lambda: len(db4.getBlockTable().get("*Model_Space").entityIds()))

# ---------------------------------------------------------------- 克隆
test("DbObject.cloneFrom", lambda: Db.Circle().cloneFrom(line))
test("Entity.clone()", lambda: line.clone())
test("Entity.getTransformedCopy", lambda: line.getTransformedCopy(Ge.Matrix3d().setToTranslation(Ge.Vector3d(1, 0, 0))))
test("Db.Database.deepCloneObjects", lambda: hasattr(db, "deepCloneObjects"))
test("Db.IdPair 类", lambda: hasattr(Db, "IdPair"))

# ---------------------------------------------------------------- 打印
test("Db.PlotSettings 类", lambda: hasattr(Db, "PlotSettings"))
test("Db.PlotSettingsValidator 类", lambda: hasattr(Db, "PlotSettingsValidator"))
test("PlotSettings.plotSettingsValidator()", lambda: hasattr(Db.PlotSettings, "plotSettingsValidator"))
test("Validator.setPlotWindowArea", lambda: hasattr(Db.PlotSettingsValidator, "setPlotWindowArea"))
test("Validator.setPlotType/setPlotRotation/setCanonicalMediaName",
     lambda: all(hasattr(Db.PlotSettingsValidator, n) for n in ("setPlotType", "setPlotRotation", "setCanonicalMediaName")))
test("Db.PlotConfigManager 类", lambda: hasattr(Db, "PlotConfigManager"))
test("Db.PlotEngine 类", lambda: hasattr(Db, "PlotEngine"))
test("Layout 继承 PlotSettings?", lambda: issubclass(Db.Layout, Db.PlotSettings))
test("Layout 有 plotSettings 入口", lambda: any(n for n in dir(Db.Layout) if "plot" in n.lower()))

print("[FT] TOTAL ok=%d fail=%d" % (sum(1 for _, o in results if o), sum(1 for _, o in results if not o)))
