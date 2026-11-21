"""功能面审计：按「交互 / 图元 / 几何 / 打印 / 读写 / 保存 / 克隆」逐项在 AutoCAD 内实测。

输出 [CAP] 行：
  PASS   真跑通
  FAIL   接口存在但调用失败（后面带异常类型/消息）
  ABSENT 接口/类缺失
"""
import os
import traceback

import PyArxCore as C

Db = C.Db
Ed = C.Ed
Ge = C.Ge
Ap = C.Ap
TMP = r"G:/PyArx/tmp_cap"
os.makedirs(TMP, exist_ok=True)


def has(path):
    """按 'Db.Line' 这种路径查属性是否存在。"""
    modname, name = path.split(".", 1)
    mod = getattr(C, modname, None)
    if mod is None:
        return False
    target = mod
    for part in name.split("."):
        if not hasattr(target, part):
            return False
        target = getattr(target, part)
    return True


def mark(kind, label, detail=""):
    print("[CAP] %-6s %-46s %s" % (kind, label, detail))


def absent(label):
    mark("ABSENT", label)


def call(label, fn):
    try:
        r = fn()
        mark("PASS", label, "" if r is None else repr(r)[:60])
        return r
    except Exception as ex:
        mark("FAIL", label, "%s: %s" % (type(ex).__name__, str(ex)[:110]))
        return None


# ---------------------------------------------------------------- 0 环境
def db_get():
    for getter in (lambda: Db.workingDb(), lambda: Ap.curDoc().database()):
        try:
            return getter()
        except Exception:
            pass
    return None


db = db_get()
mark("PASS" if db is not None else "FAIL", "Db.workingDb 取当前数据库",
     type(db).__name__ if db is not None else "拿不到")

# ---------------------------------------------------------------- 1 交互
print("[CAP] ---- 交互 ----")
INTERACT = [
    "Ed.Editor.getPoint", "Ed.Editor.getCorner", "Ed.Editor.getInt", "Ed.Editor.getReal",
    "Ed.Editor.get_string", "Ed.Editor.getKeyword", "Ed.Editor.getObjects",
    "Ed.Editor.selectImplied", "Ed.Editor.setImpliedSelection", "Ed.Editor.ssGet",
    "Ed.Editor.acedInitGet", "Ed.Editor.pointMonitorManager", "Ed.Editor.getAngle",
    "Ed.Jig", "Ed.DrawJig", "Ed.DragStyle", "Ed.DragEffect", "Ed.CursorType",
    "Ed.UserInputControls", "Ed.PromptStatus", "Ed.SelectionSet", "Ed.SelectionSetIterator",
    "Ed.InputPoint", "Ed.InputPointManager", "Ed.Util", "Ed.Editor.command",
]
for p in INTERACT:
    if not has(p):
        absent(p)

# ---------------------------------------------------------------- 2 图元
print("[CAP] ---- 图元 ----")
ENTITIES = [
    "Db.Line", "Db.Circle", "Db.Arc", "Db.Ellipse", "Db.Spline", "Db.Polyline",
    "Db.Polyline2d", "Db.Polyline3d", "Db.Text", "Db.MText", "Db.Hatch", "Db.Region",
    "Db.Solid", "Db.Solid3d", "Db.Face", "Db.BlockReference", "Db.AttributeDefinition",
    "Db.AttributeReference", "Db.Table", "Db.MLeader", "Db.Dimension", "Db.SubDMesh",
    "Db.Surface", "Db.Point", "Db.Ray", "Db.Xline", "Db.Viewport", "Db.RasterImage",
    "Db.Wipeout", "Db.BlockTableRecord", "Db.Entity", "Db.Curve",
]
for p in ENTITIES:
    if not has(p):
        absent(p)

space = None
try:
    space = Db.BlockTableRecord(db.currentSpaceId(), Db.OpenMode.kForWrite)
except Exception as ex:
    mark("FAIL", "打开当前空间 BlockTableRecord(kForWrite)", "%s: %s" % (type(ex).__name__, str(ex)[:90]))

line = None


def make_line():
    li = Db.Line()
    li.setDatabaseDefaults(db)
    li.setStartPoint(Ge.Point3d(0, 0, 0))
    li.setEndPoint(Ge.Point3d(100, 0, 0))
    space.appendAcDbEntity(li)
    return li


line = call("图元：创建 Line + setDatabaseDefaults + appendAcDbEntity", make_line)
if line is not None:
    call("图元：读回 startPoint", lambda: line.startPoint())
    call("图元：读回 endPoint", lambda: line.endPoint())
    call("图元：setLayer('0')", lambda: line.setLayer("0"))
    call("图元：getLength()", lambda: line.getLength())
    call("图元：transformBy 平移", lambda: line.transformBy(Ge.Matrix3d.translation(Ge.Vector3d(10, 0, 0))))
    call("图元：setColor(索引)", lambda: line.setColorIndex(1))
    call("图元：erase()", lambda: line.erase())

# ---------------------------------------------------------------- 3 几何
print("[CAP] ---- 几何 ----")
GEO = [
    "Ge.Point2d", "Ge.Point3d", "Ge.Vector2d", "Ge.Vector3d", "Ge.Matrix2d", "Ge.Matrix3d",
    "Ge.Plane", "Ge.Line2d", "Ge.Line3d", "Ge.LineSeg2d", "Ge.LineSeg3d",
    "Ge.CircularArc2d", "Ge.CircularArc3d", "Ge.EllipticalArc2d", "Ge.EllipticalArc3d",
    "Ge.NurbCurve2d", "Ge.NurbCurve3d", "Ge.SplineEnt2d", "Ge.SplineEnt3d",
    "Ge.Interval", "Ge.BoundBlock3d", "Ge.Tolerance", "Ge.Curve3d", "Ge.Surface",
    "Ge.Cylinder", "Ge.Sphere", "Ge.Cone", "Ge.Torus",
]
for p in GEO:
    if not has(p):
        absent(p)

call("几何：Point3d 距离", lambda: Ge.Point3d(0, 0, 0).distanceTo(Ge.Point3d(3, 4, 0)))
call("几何：Vector3d 点积", lambda: Ge.Vector3d(1, 0, 0).dotProduct(Ge.Vector3d(0, 1, 0)))
call("几何：Matrix3d 平移后变换点",
     lambda: Ge.Point3d(1, 2, 3).transformBy(Ge.Matrix3d.translation(Ge.Vector3d(1, 1, 1))))
call("几何：LineSeg3d 求交",
     lambda: Ge.LineSeg3d(Ge.Point3d(0, 0, 0), Ge.Point3d(10, 0, 0)).intersectWith(
         Ge.LineSeg3d(Ge.Point3d(5, -5, 0), Ge.Point3d(5, 5, 0))))

# ---------------------------------------------------------------- 4 打印
print("[CAP] ---- 打印 ----")
PLOT = [
    "Db.PlotSettings", "Db.PlotSettingsValidator", "Db.PlotStyleTable", "Db.PlotStyleTableRecord",
    "Db.Layout", "Db.LayoutManager", "Db.PlotType", "Db.PlotPaperUnits", "Db.PlotRotation",
]
for p in PLOT:
    if not has(p):
        absent(p)
if has("Db.Database.plotSettingsValidator"):
    call("打印：db.plotSettingsValidator()", lambda: db.plotSettingsValidator())

# ---------------------------------------------------------------- 5 读写
print("[CAP] ---- 读写 ----")
IO = [
    "Db.Database.readDwgFile", "Db.Database.readDwgAndClose", "Db.Database.saveAs",
    "Db.Database.dxfIn", "Db.Database.dxfOut", "Db.dxfOutAs2000", "Db.dxfOutAs2004",
    "Db.dxfOutAsR12", "Db.Database.blockTable", "Db.Database.currentSpaceId",
]
for p in IO:
    if not has(p):
        absent(p)

dwg_out = os.path.join(TMP, "cap_out.dwg")


def save_as():
    db.saveAs(dwg_out)
    return os.path.exists(dwg_out)


call("保存：db.saveAs(临时 dwg)", save_as)

dxf_out = os.path.join(TMP, "cap_out.dxf")


def dxf_out_call():
    Db.dxfOutAs2004(db, dxf_out, 16)
    return os.path.exists(dxf_out)


if has("Db.dxfOutAs2004"):
    call("读写：Db.dxfOutAs2004 导出 dxf", dxf_out_call)

# ---------------------------------------------------------------- 6 克隆
print("[CAP] ---- 克隆 ----")
CLONE = [
    "Db.DbObject.clone", "Db.Entity.clone", "Db.deepCloneObjects", "Db.wblockCloneObjects",
    "Db.DbObject.cloneFrom", "Db.IdPair", "Db.DeepCloneType", "Db.DbObject.cloneFrom",
]
for p in CLONE:
    if not has(p):
        absent(p)


def clone_line():
    li = Db.Line()
    li.setDatabaseDefaults(db)
    li.setStartPoint(Ge.Point3d(0, 50, 0))
    li.setEndPoint(Ge.Point3d(100, 50, 0))
    space.appendAcDbEntity(li)
    cp = li.clone()
    return type(cp).__name__


call("克隆：Line.clone()", clone_line)


def clone_from():
    src = Db.Line()
    src.setDatabaseDefaults(db)
    src.setStartPoint(Ge.Point3d(0, 60, 0))
    src.setEndPoint(Ge.Point3d(100, 60, 0))
    dst = Db.Line()
    dst.cloneFrom(src)
    return dst.endPoint().x


call("克隆：cloneFrom 拷贝几何", clone_from)

print("[CAP] DONE")
