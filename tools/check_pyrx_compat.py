"""PyRx 兼容层（P0）CAD 内自检。

在 AutoCAD 里执行（PyArx 已加载 PyArxCore 时）：

    import runpy
    runpy.run_path(r"G:\\PyArx\\tools\\check_pyrx_compat.py")

只用 PyRx 的短名路径，验证「PyRx 风格调用是否寻址成功」，不触发真实绘图操作。
"""
import sys

import PyArxCore as C
from PyArxCore import Db, Ge, Ed, Gi, Rx, Ap

fails, ok = [], 0


def has(mod, name, label=""):
    global ok
    if hasattr(mod, name):
        ok += 1
    else:
        fails.append("%s%s.%s" % (label, getattr(mod, "__name__", "?"), name))


# 1 类名短名（每模块抽查 + 计数）
CASES = [
    (Db, ["Polyline", "BlockReference", "Database", "Line", "Circle", "MText",
          "LayerTable", "LayerTableRecord", "Spline", "Region", "Table"]),
    (Ge, ["Point3d", "Point2d", "Vector3d", "Matrix3d", "Matrix2d", "Line3d", "Plane"]),
    (Ed, ["SelectionSet"]),
    (Gi, ["Geometry", "WorldDraw", "WorldGeometry"]),
    (Rx, ["RxObject", "RxClass"]),
    (Ap, ["Application", "DocumentManager", "PaletteSet"]),
]
for mod, names in CASES:
    for n in names:
        has(mod, n)

# 2 统计实际别名数量（PyRx 名存在但 PyArx 原名也存在的成对计数）
pairs = [(Db, "Polyline", "PyDbPolyline"), (Ge, "Point3d", "AcGePoint3d"),
         (Ap, "Application", "PyApp")]
for mod, short, orig in pairs:
    if not (hasattr(mod, short) and hasattr(mod, orig)):
        fails.append("成对缺失: %s/%s" % (short, orig))

# 3 跨子模块枚举：PyRx 在 Db 下，PyArx 实际在 Enum
ENUMS = ["ErrorStatus", "DxfCode", "OpenMode", "UnitsValue", "LineWeight",
         "JoinStyle", "EndCaps", "Intersect", "Visibility", "MeasurementValue"]
for n in ENUMS:
    has(Db, n, "Db.")

# 4 Ed.Core / Db.Core 静态函数类
for n in ["alert", "arxLoad", "arxLoaded", "arxUnload", "prompt", "getVar", "setVar",
          "cmdS", "trans", "vports"]:
    has(Ed.Core, n, "Ed.Core.")
# PyRx 的 PyDb.pyi 没有 Core 类，Db.Core.* 不是 PyRx 接口；curDb 是 Db 模块级函数
has(Db, "curDb", "Db.")

# 5 模块级函数
for n in ["curDb", "workingDb", "setWorkingDb"]:
    has(Db, n, "Db.")
for n in ["curDoc", "using_scope"]:
    has(Ap, n, "Ap.")
pending = [
    "Ap.Command / Ap.LispFunction / Ed.print（按需求不实现）",
    "Ed.Core.cmdUndefine（ARX 2024 SDK 不再导出 acedCmdUndefine）",
    "Ed.EditorReactor 事件派发未接线（类骨架已提供）",
]

# 6 行为抽查：不建图元，只读
try:
    wdb = Db.workingDb()
    assert isinstance(wdb, Db.Database), type(wdb)
except Exception as e:                      # noqa: BLE001
    fails.append("Db.workingDb() 调用失败: %r" % (e,))

# angToS/angToF 是 PyArx 独有（acdbAngToS/acdbAngToF），PyRx Ed.Core 无此成员，这里不做断言

# using_scope 是装饰器：作用在只读函数上，验证不抛异常
try:
    @Ap.using_scope()
    def _probe():
        return 1
    assert _probe() == 1
except Exception as e:                      # noqa: BLE001
    fails.append("Ap.using_scope() 装饰器调用失败: %r" % (e,))

# P1：原生 AcDbObjectId / AcDbHandle 直接绑定（PyRx 名 ObjectId / Handle）
for n in ["ObjectId", "Handle"]:
    has(Db, n, "Db.")
try:
    oid = Db.ObjectId(0)
    assert oid.isNull() and oid.asOldId() == 0 and int(oid) == 0, "空 id 语义不符"
    oid2 = Db.ObjectId.fromOldId(5)
    assert oid2.asOldId() == 5, "fromOldId 往返失败"
    oid3 = Db.ObjectId(7)
    oid3.setFromOldId(9)
    assert oid3.asOldId() == 9, "setFromOldId 失败"
    import operator
    assert operator.index(oid3) == 9, "__index__ 缺失，ObjectId 无法透传给 uint64 入参"
    assert Db.ObjectId(5) == 5 and Db.ObjectId(5) == Db.ObjectId(5), "与 int/ObjectId 比较失败"
except Exception as e:                      # noqa: BLE001
    fails.append("Db.ObjectId 构造/往返失败: %r" % (e,))
try:
    h = Db.Handle("1")
    assert h.toString().endswith("1"), "Handle 字符串往返失败: %r" % (h.toString(),)
    h2 = Db.Handle(1, 0)
except Exception as e:                      # noqa: BLE001
    fails.append("Db.Handle 构造/toString 失败: %r" % (e,))

# P1：Extents / Extents2d / ObjectIdArray / IdPair
for n in ["Extents", "Extents2d", "ObjectIdArray", "IdPair"]:
    has(Db, n, "Db.")
try:
    e3 = Db.Extents()
    assert e3.isEmpty(), "空 Extents 应为 isEmpty"
    e3.addPoint(__import__("PyArxCore").Ge.Point3d(1, 2, 3))
    e3.addPoint(__import__("PyArxCore").Ge.Point3d(4, 5, 6))
    assert not e3.isEmpty(), "addPoint 后不应为空"
    mn, mx, mid = e3.minPoint(), e3.maxPoint(), e3.midPoint()
    assert (mn.x, mn.y, mn.z) == (1, 2, 3), "minPoint 不符"
    assert (mx.x, mx.y, mx.z) == (4, 5, 6), "maxPoint 不符"
    assert (mid.x, mid.y, mid.z) == (2.5, 3.5, 4.5), "midPoint 不符"
    assert len(e3.coords()) == 6, "coords 应返回 6 元组"
    assert e3.contains(__import__("PyArxCore").Ge.Point3d(2, 3, 4)), "contains 失配"
    e3b = Db.Extents()
    e3b.set(__import__("PyArxCore").Ge.Point3d(3, 4, 5), __import__("PyArxCore").Ge.Point3d(7, 8, 9))
    assert e3.intersectsWith(e3b), "intersectsWith 失配"
    assert e3 == e3, "Extents 自等失败"
except Exception as ex:                     # noqa: BLE001
    fails.append("Db.Extents 行为失败: %r" % (ex,))
try:
    e2 = Db.Extents2d()
    e2.addPoint(__import__("PyArxCore").Ge.Point2d(1, 2))
    e2.addPoint(__import__("PyArxCore").Ge.Point2d(3, 4))
    assert (e2.minPoint().x, e2.minPoint().y) == (1.0, 2.0), "Extents2d.minPoint 不符"
except Exception as ex:                     # noqa: BLE001
    fails.append("Db.Extents2d 行为失败: %r" % (ex,))
try:
    # ObjectIdArray 走 PyArrayCaster.h 的既有 caster：Python 侧就是 oldId 的 list
    arr = Db.ObjectIdArray()
    assert isinstance(arr, list) and len(arr) == 0, "ObjectIdArray 应映射为 list"
    arr.append(3)
    arr.append(1)
    arr.extend([2])
    assert len(arr) == 3 and arr[0] == 3, "list 语义基本操作失败"
    assert 1 in arr, "__contains__ 失败"
    arr.reverse()
    assert arr[0] == 2, "reverse 失败"
    arr.sort()
    assert arr == [1, 2, 3], "sort 失败"
    popped = arr.pop()
    assert popped == 3 and len(arr) == 2, "pop 失败"
    arr.clear()
    assert len(arr) == 0, "clear 失败"
except Exception as ex:                     # noqa: BLE001
    fails.append("Db.ObjectIdArray 行为失败: %r" % (ex,))
try:
    p = Db.IdPair(Db.ObjectId(1), Db.ObjectId(2))
    assert p.first.asOldId() == 1 and p.second.asOldId() == 2, "IdPair 字段不符"
except Exception as ex:                     # noqa: BLE001
    fails.append("Db.IdPair 行为失败: %r" % (ex,))

# P1：Ed.Editor 转发 + acedCoordFromPixelToWorld
for n in ["Editor", "EditorReactor"]:
    has(Ed, n, "Ed.")
for n in ["className", "getPoint", "getString", "getInteger", "getReal", "getKword",
          "getCorner", "getDist", "getAngle", "entSel", "getCurrentUCS", "writeMessage"]:
    if not hasattr(Ed.Editor, n) or n not in dir(Ed.Editor):
        fails.append("Ed.Editor 缺成员: %s" % n)
    else:
        ok += 1
has(Ed, "acedCoordFromPixelToWorld", "Ed.")
try:
    assert Ed.Editor.className() == "Editor", "Editor.className 不符"
    Ed.EditorReactor()
except Exception as ex:                     # noqa: BLE001
    fails.append("Ed.Editor.className/Reactor 构造失败: %r" % (ex,))

# P1：Db.Field + 5 组字段枚举
for n in ["Field", "FieldState", "FieldEvalOption", "FieldEvalContext", "FieldEvalStatus", "FieldCodeFlag"]:
    has(Db, n, "Db.")
try:
    fld = Db.Field()
    assert Db.Field.className() == "AcDbField", "Field.className 不符"
    assert fld.childCount() == 0, "新字段 childCount 应为 0"
    assert fld.isTextField() is False, "新字段不应是 text field"
    fld2 = Db.Field("%<\\AcVar Date \\f\"%tc1\">%")
    assert fld2.getFieldCode(Db.FieldCodeFlag.kFieldCode) is not None, "getFieldCode 调用失败"
except Exception as ex:                     # noqa: BLE001
    fails.append("Db.Field 行为失败: %r" % (ex,))

# 枚举互操作：py::enum_ 成员 / Python IntEnum / 裸 int 三种写法都能进 int 形参
try:
    import enum

    class _ColorType(enum.IntEnum):
        r = 0
        g = 1
        b = 2

    assert hasattr(Db.FieldCodeFlag.kFieldCode, "__index__"), "py::enum_ 成员应实现 __index__"
    assert int(Db.FieldCodeFlag.kFieldCode) == 1, "枚举成员转 int 失败"
    fld3 = Db.Field()
    assert fld3.getFieldCode(Db.FieldCodeFlag.kFieldCode) is not None      # 枚举成员 -> int
    assert fld3.getFieldCode(_ColorType.g) is not None                     # Python IntEnum -> int
    assert fld3.getFieldCode(1) is not None                                # 裸 int -> int
    assert int(Db.FieldCodeFlag.kFieldCode) == 1 and int(_ColorType.b) == 2
except Exception as ex:                     # noqa: BLE001
    fails.append("枚举互操作失败: %r" % (ex,))

print("[PyRxCompat] 通过 %d 项，失败 %d 项" % (ok, len(fails)))
for f in fails:
    print("  FAIL", f)
for p in pending:
    print("  TODO", p)
if not fails:
    print("[PyRxCompat] P0 别名层全部命中")
