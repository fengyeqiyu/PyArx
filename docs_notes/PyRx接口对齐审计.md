# PyArxCore ↔ PyRx 接口对齐审计

- 审计日期：2026-11-17
- 范围：PyArxCore 全部导出接口 vs PyRx 全部公开接口，**排除 wx**
- 性质：纯静态、机器化、可复现。脚本 `tools/api_parity_audit.py`，明细 CSV 在 `docs_notes/api_parity/`
- 复现：`cd G:\PyArx; G:\PyArx\X64\Release\python.exe tools\api_parity_audit.py`（重跑覆盖 CSV）

---

## 1 结论摘要

| 指标 | 数值 |
|---|---|
| PyRx 桩模块总数 | 14 个，1,109 类（1,905KB 的 PyDb.pyi 最大） |
| 本次逐类比对的模块 | 6 个（PyDb/Ge/Ed/Gi/Rx/Ap），544 类 |
| 其余 8 个模块（Gs/Pl/Sm/Ax/Br/Brx/BrxBim/BrxCv） | 565 类，整体缺失，**有效对应仅 1 个类**（见 §5.1） |
| 全量口径（14 模块） | 类 **232/1,109（20.9%）**，成员 **4,119/19,359（21.3%）** |
| PyRx 六模块类数（PyDb/Ge/Ed/Gi/Rx/Ap） | 544（含枚举） |
| 已对齐类数 | **231（42%）**，其中类名完全一致仅 **29** |
| PyRx 成员总数（限已对齐类） | 6,472 |
| 已对齐成员 | **4,098（63.3%）**，缺 **2,374**，PyArx 多出 908 |
| 整类缺失 | **313**（枚举 172 + 类 141） |
| 基类继承不一致 | 27 |
| 模块级函数 | PyRx 9 个，PyArxCore 仅 1 个（`Db.curDb`） |
| PyArxCore 实际注册 | 300 类 + 70 枚举，类内成员与枚举值约 6,600 |
| PyArx 独有类 | 103（多为 AcGi* 内部件，非缺口） |

> 一句话：**不是"少几个函数"，而是三层错位——类名体系、承载子模块、基础类型**。PyRx 的代码无法靠改 `import` 名在 PyArxCore 上跑通。

三条硬结论：

1. **命名层**：PyArxCore 用 `PyDbPolyline` / `AcGePoint3d` / `PyApp`，PyRx 用 `Db.Polyline` / `Ge.Point3d` / `Ap.Application`。544 个类里只有 29 个同名（Db 20、Ed 6、Ap 3）。
2. **类型层**：PyArxCore **没有 `ObjectId` 类型**，全库以 `uint64_t` 直传对象 id；PyRx 侧 `ObjectId` 是 25 个成员的对象类型。同理缺 `Handle`、`Extents`/`Extents2d`、`IdPair`、`ObjectIdArray`、`Field`、`PointCloud*`。这是最深的一层不兼容，改不完类型层，其余对齐都是表面功夫。
3. **承载层**：PyRx 把 `acedXxx` 收进 `Ed.Core` / `Db.Core` 静态类并**去掉 aced 前缀**（`alert`/`arxLoad`/`cmdS`）；PyArxCore 是 `Ed` 模块级 `acedXxx` 函数。另有一批枚举 PyRx 放在 `Db` 下，PyArxCore 放在独立的 `Enum` 子模块。

---

## 2 逐模块覆盖率

| 模块 | PyRx 类 | 对齐类 | 命名一致 | 成员对齐 | 整类缺（枚举/类） | 基类不一致 |
|---|---|---|---|---|---|---|
| PyDb | 372 | 142 (38%) | 20 | 3,220 / 4,912 (66%) | 230（146 / 84） | 20 |
| PyGe | 86 | 51 (59%) | 0 | 610 / 1,065 (57%) | 35（12 / 23） | 5 |
| PyEd | 27 | 9 (33%) | 6 | 104 / 127 (82%) | 18（3 / 15） | 0 |
| PyGi | 31 | 22 (71%) | 0 | 143 / 180 (79%) | 9（4 / 5） | 0 |
| PyRx | 7 | 2 (29%) | 0 | 7 / 28 (25%) | 5（2 / 3） | 0 |
| PyAp | 21 | 5 (24%) | 3 | 14 / 160 (9%) | 16（5 / 11） | 2 |
| **合计** | **544** | **231 (42%)** | **29** | **4,098 / 6,472 (63%)** | **313** | **27** |

模块级函数：PyRx 共 9 个（`PyDb.curDb`/`workingDb`/`setWorkingDb`、`PyEd.print`、`PyAp.Command`/`LispFunction`/`curDoc`/`fireOnbeginConsole`/`using_scope`），PyArxCore **只命中 1 个（`Db.curDb`，`PyDbDatabase.cpp:2793` 的 `m.def`）**，其余 8 个缺失。

---

## 3 三层结构性差异（带证据）

### 3.1 类名体系

| 子模块 | PyRx 名 | PyArxCore 注册名 | 量级 |
|---|---|---|---|
| Db | `Db.Polyline`、`Db.BlockReference`、`Db.Database` | `PyDbPolyline`、`PyDbBlockReference`、`PyDbDatabase` | Db 子模块共 171 个注册类型，绝大多数带 `PyDb` 前缀 |
| Ge | `Ge.Point3d`、`Ge.Matrix3d` | `AcGePoint3d`、`AcGeMatrix3d` | 51 类带 `AcGe` 前缀 |
| Gi | `Gi.Geometry`、`Gi.WorldDraw` | `PyGiGeometry`、`PyGiWorldDraw`、`AcGiVariant` | 61 类，前缀 PyGi/AcGi 混用 |
| Ap | `Ap.Application`、`Ap.DocumentManager` | `PyApp`、`DocumentManager`、`PaletteSet` | 6 类 |
| Ed | `Ed.Editor`、`Ed.Core` | `PyEdResult`、`AcEdJig`、`SelectionSet` | 8 类 |
| Enum 混装 | 无独立 Enum 模块 | `Enum.ErrorStatus`、`Enum.DxfCode`… | 27 枚举 |

`name_mismatch.csv` 有 202 条逐条映射（module, pyrx_name, pyarx_registered_name, how），可直接作为别名层数据源。

### 3.2 承载子模块错位（改名之外还要换位置）

PyRx 在 `Db` 下、PyArxCore 实际落在别处（同名同物，只是父模块不同）：

`wrong_module.csv` 精确记录了 19 条"同名但父模块不同"：

- PyDb 侧 15 个枚举落在 `Enum` 子模块：`ErrorStatus`(511 值)、`DxfCode`(169 值)、`OpenMode`、`UnitsValue`、`LineWeight`、`JoinStyle`、`EndCaps`、`MeasurementValue`、`UpdateOption`、`UpdateDirection`、`Intersect`、`Visibility`、`PlotStyleNameType`
- PyEd 侧 5 个枚举落在 `Enum`：`PromptStatus`、`CursorType`、`DragStatus`、`DragStyleType`、`UserInputControls`
- `Db.AdsName` 实际在 `Ed` 子模块
- 颜色/透明度：`Db.ColorMethod`/`EntityColor`/`Transparency`/`TransparencyMethod` → PyArxCore **子模块对、名字错**（`AcCmColorMethod`/`AcCmEntityColor`/`AcCmTransparency`/`AcCmTransparencyMethod`），已计入 `name_mismatch.csv`，只需别名不需改结构
- `Ed.Core`（170 成员）、`Db.Core`（117 成员）两个静态工具类 **完全缺失**

### 3.3 基础值类型缺位（最深不兼容）

| PyRx 类型 | PyArxCore 现状 | 影响面 |
|---|---|---|
| `Db.ObjectId`（25 成员） | **无此类型**，全部 `uint64_t` | 所有返回/接受 id 的 API |
| `Db.Handle`（16 成员） | 无 | `DbObject.handle()` 返回字符串 |
| `Db.Extents`(15) / `Extents2d` | `PyDbExtents.h` 有声明、**无 .cpp、未注册** | 几何范围查询 |
| `Db.IdPair`(13)、`ObjectIdArray`(16) | 无（用 py::list） | 选择集、组、字典遍历 |
| `Db.HardPointerId` / `SoftPointerId` | 无 | XData/XRecord 类型标记 |
| `Db.Field`（32 成员） | 无 | 字段表达式 |
| `Db.LayerFilter`(23) / `TransactionManager`(6) | 无（有 `PyDbFilter` 但非同一物） | 图层过滤、事务管理 |
| `Db.PointCloudEx` / `PointCloudCrop` / … | 无（仅 GeoData） | 点云 |
| `Db.AssocAction` 系 | 无 | 约束/关联 |
| `Db.Table`/`Cell`/`CellRange`/`CellStyle` 单元格 API | 仅 `PyDbTable` 基础行（缺 149 成员） | 表格 |
| `Db.SectionSettings`/`SectionManager` | 无 | 截面 |

id 传递的直接证据：

```
PyArxCore/PyDbDatabase.h:26-33
  AcDbObjectId addAcDbObject(PyDbObject& pObj);
  uint64_t addToModelspace(PyDbObject& entity);
  uint64_t addToCurrentspace(PyDbObject& entity);
  uint64_t addToBlock(uint64_t blockId, PyDbObject& entity);
PyArxCore/PyDbDatabase.h:74
  PyDbObject openObject(uint64_t objectId, int mode = 1);
PyArxCore/PyDbBlockReference.h:15-16
  uint64_t blockTableRecordId() const;
  bool setBlockTableRecord(uint64_t blockRecordId);
```

PyRx 同一位置返回 `ObjectId`，可继续 `.getObject()` / `.handle()` / `.isNull()`。**只补成员不补类型，等于接口长得像但用不起来。**

### 3.4 继承关系不一致（27 处）

`isinstance` / MRO 层面直接失效的典型：

- `Db.DbObject` 应继承 `Gi.Drawable`，PyArxCore 的 `PyDbObject` 直接挂在 `PyRxObject` 上
- `Db.Layout` 应继承 `Db.PlotSettings`，实际只继承 `PyDbObject`
- `Db.PdfReference` 应继承 `Db.UnderlayReference`、`Db.PdfDefinition` 应继承 `Db.UnderlayDefinition`，实际都退化为 `Entity`/`DbObject`
- `Db.RasterImage` 应继承 `Db.Image`，实际 `PyDbEntity`
- `Db.AnnotationScale` 应继承 `Db.ObjectContext`，实际无基类
- `Db.DatabaseReactor`/`DatabaseSummaryInfo`/`EvalVariant`/`ObjectContextManager`/`ObjectIdGraph` 应继承 `RxObject`，实际无基类

### 3.5 aced 函数承载与改名

- PyArxCore：`Ed.acedPrompt`、`Ed.acedGetVar`、`Ed.acedAlert`… 共 **80 个去重后的 aced\* 模块级函数**
- PyRx：`Ed.Core.alert`、`Ed.Core.arxLoad`、`Ed.Core.arxLoaded`、`Ed.Core.cmdS`、`Ed.Core.coordFromPixelToWorld`；`Db.Core` 另有 `activeDatabaseArray`/`angToF`/`attachXref`/`bindXrefs` 等
- 用 PyRx 桩 docstring 里的 aced 原名反查：**32 个能定位到 PyRx 短名**（`acedArxLoad`→`Core.arxLoad`、`acedArxLoaded`→`Core.arxLoaded`、`acedArxUnload`→`Core.arxUnload` …），**48 个 PyRx 本身未导出**，属 PyArx 增量而非缺口
- 映射明细 `aced_mapping.csv`；因 docstring 邻近文本存在噪声，个别条目（如 `acedCmd`→`Core.entLast`）需人工复核

---

## 4 缺口最集中的 20 个类（成员缺口排序）

| PyRx 类 | PyArxCore 注册名 | PyRx 成员 | 已对齐 | 缺 | 多 |
|---|---|---|---|---|---|
| PyDb.Database | PyDbDatabase | 511 | 137 | 374 | 44 |
| PyDb.Table | PyDbTable | 187 | 38 | 149 | 5 |
| PyDb.DimStyleTableRecord | PyDbDimStyleTableRecord | 163 | 66 | 97 | 13 |
| PyDb.Dimension | PyDbDimension | 118 | 37 | 81 | 30 |
| PyDb.Viewport | PyDbViewport | 167 | 88 | 79 | 2 |
| PyDb.MLeader | PyDbMLeader | 119 | 47 | 72 | 4 |
| PyDb.SubDMesh | PyDbSubDMesh | 87 | 29 | 58 | 2 |
| PyAp.PaletteSet | PaletteSet | 50 | 0 | 50 | 6 |
| PyDb.GeoData | PyDbGeoData | 60 | 12 | 48 | 2 |
| PyAp.DocManager | DocumentManager | 43 | 7 | 36 | 5 |
| PyAp.Application | PyApp | 34 | 0 | 34 | 13 |
| PyDb.Spline | PyDbSpline | 61 | 32 | 29 | 4 |
| PyDb.TableStyle | PyDbTableStyle | 75 | 46 | 29 | 2 |
| PyDb.DbObject | PyDbObject | 76 | 49 | 27 | 16 |
| PyDb.Polyline | PyDbPolyline | 57 | 30 | 27 | 10 |
| PyDb.Section | PyDbSection | 47 | 20 | 27 | 2 |
| PyDb.Surface | PyDbSurface | 32 | 5 | 27 | 2 |
| PyGe.Matrix3d | AcGeMatrix3d | 62 | 36 | 26 | 4 |
| PyGe.Matrix2d | AcGeMatrix2d | 49 | 24 | 25 | 7 |
| PyGi.Geometry | PyGiGeometry | 47 | 22 | 25 | 15 |

`Polyline` 的 27 个缺口不是命名差异：PyRx 的风格化属性（`bulge`/`index`/`startWidth`/`endWidth`/`point2d`）与同名方法（`getPoint2dAt`/`getStartWidthAt`/`getArcSeg2dAt`/`getAcGeCurve`）在 PyArxCore 里**两者都没有**，说明 `PyDbPolyline` 目前只是基础子集（30/57）。

---

## 5 整类缺失 313 项的分类

- **空壳大件（PyRx 独有、PyArx 完全没有）**：`Ed.Core`(170)、`Db.Core`(117)、`Ed.EditorReactor`(91)、`Ed.Editor`(67)、`Db.PointCloudEx`(90)、`Db.AssocAction`(88)、`Db.Solid3d`(77)、`Db.AbstractViewTableRecord`(59)
- **枚举批量**：`Ge.EntityId`(81)、`Db.TimeZone`(76)、`Db.GeoCoordinateSystemProjectionCode`(72)、`Db.GeoCoordinateSystemUnit`(62) … 合计 172 个枚举缺失，占整类缺失的 55%
- **整模块**：`PyGs` 6 类、`PyPl` 27 类、`PySm` 32 类（AutoCAD 正式 API，可实现）；`PyAx` 232 类（COM 自动化）、`PyBr` 38 / `PyBrx` 22 / `PyBrxBim` 56 / `PyBrxCv` 152（BricsCAD 专有，**AutoCAD 上不存在对应运行时**）

### 5.1 其余 8 个模块已逐一核对（不是简单判"缺"）

PyRx 共 14 个桩模块。本次逐类比对了其中 6 个（544 类），另 8 个按同一口径扫过一遍（宽松匹配 = 任意子模块同名/同类型都算命中），结果见 `module_inventory.csv` / `extra_module_matches.csv`：

| 模块 | 类数 | 名义命中 | 成员数 | 成员对齐 | 性质 | 结论 |
|---|---|---|---|---|---|---|
| PyGs | 6 | 0 | 54 | 0 | AutoCAD 正式 API（AcGs 视图/显示） | 整体缺失，需新建 gs 子模块 |
| PyPl | 27 | 0 | 327 | 0 | AutoCAD 正式 API（打印/发布） | 整体缺失：PyRx 的 `PlotInfo`/`PlotEngine`/`PlotConfig`/`PlotConfigManager`/`PlotProgressDialog`/`DSDData` 在 PyArxCore 全无；现有 `PyDbPlotSettings`/`PyDbPlotSettingsValidator` 是 Db 侧的页面设置对象，不属于打印管线 |
| PySm | 32 | 1（噪声） | 273 | 2 | AutoCAD 正式 API（图纸集） | 整体缺失；`PySm.DbDatabase` 仅字面撞上 `PyDbDatabase` |
| PyAx | 232 | 1 | 4,340 | 0 | COM 自动化（AcAx） | 仅 `PyAx.AcadDatabase → PyAcadDatabase` 是真实雏形（3 成员、0 个共同成员） |
| PyBr | 38 | 4（噪声） | 296 | 19 | BricsCAD 专有 | 名义命中的 `Entity`/`Face`/`Vertex`/`ErrorStatus` 全是字面冲突 |
| PyBrx | 22 | 0 | 195 | 0 | BricsCAD 专有 | 整体缺失 |
| PyBrxBim | 56 | 0 | 1,649 | 0 | BricsCAD 专有 | 整体缺失 |
| PyBrxCv | 152 | 1（噪声） | 1,891 | 0 | BricsCAD 专有（土木） | 名义命中的 `ArcType` 是字面冲突 |
| **小计** | **565** | **7** | **9,025** | **21** | — | 实际有效对应仅 1 个类（0.2%） |

反向也有需要记录的一侧：PyArxCore 有 3 个 PyRx 没有的子模块——`Core`（22 个模块级函数）、`Utility`（2）、`Debug`（4）；另有 `Enum` 子模块（27 枚举）。这些是 PyArx 独有，不是缺口。

---

## 6 对齐路线（按投入产出排序）

### P0 命名与承载别名层（约 1 天，纯增量，不改任何现有绑定）

目标是让 **PyRx 代码改 import 后能跑通形状**，成员缺口留到 P2。

1. `PyArxBinder.cpp` 末尾为每个已注册类加短名别名（数据源 `name_mismatch.csv` 202 条）：
   `m.attr("Polyline") = m.attr("PyDbPolyline")`、`m.attr("Point3d") = m.attr("AcGePoint3d")` …
2. 枚举回到 PyRx 的位置：`Db.attr("ErrorStatus") = Enum.attr("ErrorStatus")`（同理 `DxfCode`/`UnitsValue`/`JoinStyle`… 约 30 条）
3. 新增 `PyDb.Core` / `PyEd.Core` 两个 `py::class_` 静态类，方法名去掉 `aced` 前缀转发到现有模块函数（`aced_mapping.csv` 已给出 32 条直接映射，其余 48 个保持 `aced` 原名并以别名补齐）
4. 补 8 个模块级函数：`Db.workingDb`、`Db.setWorkingDb`、`Ed.print`、`Ap.Command`、`Ap.LispFunction`、`Ap.curDoc`、`Ap.fireOnbeginConsole`、`Ap.using_scope`（contextmanager 用 py::with 实现）；`Db.curDb` 已有，只需保持
5. 继承补齐：`PyDbObject` 增加 `PyGiDrawable` 基、`PyDbLayout` 增加 `PyDbPlotSettings` 基等 27 处

P0 完成后：231 个类、4,098 个成员可被 PyRx 风格代码直接命中（覆盖率不变，但"可寻址"从 0 到 63%）。

#### P0.1 落地状态（2026-11-17，已编译通过）

| 项 | 实现 | 数量 |
|---|---|---|
| 类名短名别名 | `PyArxCore/PyArxCompat.cpp`（`A(m, to, from)`） | **202 条**（PyDb 142 / PyGe 51 / PyEd 9 / PyGi 22 / PyRx 2 / PyAp 5） |
| 跨子模块枚举回挂 | `A2(m, to, src, from)`，`Db.ErrorStatus = Enum.ErrorStatus` … | **19 条**（含 `AdsName → Ed`） |
| `Ed.Core` / `Db.Core` 静态工具类 | `def_submodule("Core")` + short→aced 转发 | **23 条**（`arxLoad`/`arxLoaded`/`arxUnload`/`prompt`/`getVar`/`setVar`/`trans`/`vports`…） |
| 模块级函数 | `Db.workingDb()`、`Db.setWorkingDb()` | 2 个（`Db.curDb` 原有） |
| Ap SDK 封装 | `Ap.curDoc()`（`::curDoc()` → `PyApDocument`，空抛 `eNoDocument`）；`Ap.using_scope()`（GIL 作用域装饰器） | 2 个 |
| 基础类型 | `Db.ObjectId` / `Db.Handle`：**直接绑定原生 `AcDbObjectId` / `AcDbHandle`**,不套包装类；`asOldId`/`setFromOldId`/`fromOldId`/`__int__`/`__index__` 与既有 `uint64_t` 口径互通，`__eq__` 可同 `int` 比较 | 2 个类 |

> `__index__` 的作用（关键设计）：pybind11 整型 caster 在 CPython>=3.8 走 `PyIndex_Check` → `PyLong_AsLongLong` → `__index__`（`third_party/pybind11/include/pybind11/cast.h:141`），因此**现有 173 处 `uint64_t` 入参无需逐个加重载**即可直接接受 `ObjectId`。反向（返回 `uint64_t` 的函数）暂不改为 `ObjectId`：`oid + 1` 等算术会失效，留待确认后按出口点分批切换。

- 代码由 `tools/gen_pyrx_compat.py` 从审计 CSV 生成，重跑即可再生成；只在源名存在且目标名不存在时追加，**不覆盖任何既有绑定**
- 入口挂在 `PyArxBinder.cpp` 末尾（`bind_PyArxCompat(m)`，异常隔离，失败不影响主模块）
- 编译：`MSBuild PyArxCore.vcxproj /p:Configuration=Release /p:Platform=x64` → `X64\Release\PyArxCore.pyd`（8.72MB，2026-11-17 19:27）
- CAD 内自检：`runpy.run_path(r"G:\PyArx\tools\check_pyrx_compat.py")`

**未落地（经评估不实现，2026-11-17 定案）**：
- `Ed.print`、`Ap.Command`、`Ap.LispFunction`：命令注册/装饰器语义，按用户决定不封
- `Ap.fireOnbeginConsole()`：PyRx 侧是 Win32 控制台窗口保护（`GetConsoleWindow` + `GetSystemMenu` 禁 `SC_CLOSE` + `SetConsoleCtrlHandler`，见 `PyRxCore/PyAcAp.cpp:39`），与 ObjectARX SDK 无关；PyArx 输出走 CAD 命令行、无独立控制台窗口，无封装对象
- 27 处基类补齐要改各 `py::class_` 模板参数，风险高于别名，放到 P1 一起做

### P1 基础值类型（约 2–3 天，必须先做，否则 P2 白做）

`ObjectId`（`Db.ObjectId`）、`Handle`、`Extents`/`Extents2d`（补 `PyDbExtents.cpp`）、`IdPair`、`ObjectIdArray`、`Field`、`DictionaryIterator`。

关键设计：`ObjectId` 必须 **与 `uint64_t` 双向兼容**——`py::init<uint64_t>()` + `__int__`，否则会打断 PyArx 现有全部 id 用法（`addToModelspace` 返回 int 的调用点）。改造 6 个 id 出口：`Database.addToModelspace/addToCurrentspace/addToBlock`、`BlockReference.blockTableRecordId/setBlockTableRecord/openAttribute`、`Database.openObject`。

### P2 成员补齐（分批，按使用频次，不按字母序）

- 批 1（绘图主干）：`Polyline` 27、`BlockReference` 17、`Entity` 26、`Line` 10、`DbObject` 27
- 批 2（注释/标注）：`Dimension` 81、`MLeader` 72、`DimStyleTableRecord` 97、`Table` 149
- 批 3（几何/曲面/点云）：`SubDMesh` 58、`Surface` 27、`Spline` 29、`GeoData` 48、`Matrix3d` 26、`Matrix2d` 25
- 批 4（应用层）：`Application` 34、`DocManager` 36、`PaletteSet` 50、`EditorReactor` 91

约 2,374 个成员，估算 3,000–3,500 行 `.def` 代码。

### P3 整模块

| 模块 | 类数 | 判断 |
|---|---|---|
| PyGs | 6 | 可实现（AcGs 视图/显示管理），估 2–3 天 |
| PyPl | 27 | 可实现（打印/发布），估 1–2 周，含 DSD 数据 |
| PySm | 32 | 可实现（AcSm 图纸集），估 2 周 |
| PyAx | 232 | 需 AcAx 头 + COM 编组，估 3–4 周，且与"纯 pybind11"路线冲突 |
| PyBr/PyBrx/PyBrxBim/PyBrxCv | 268 | **AutoCAD 无对应 API**，建议判"不适用" |

---

## 7 范围裁决（需你拍板的三件事）

1. **PyBr 系 268 类**（Br 38 / Brx 22 / BrxBim 56 / BrxCv 152）：BricsCAD 专有，AutoCAD ARX 无运行时。按"除 wx 外全对齐"的字面要求必须列为缺口，但实现不成立（名义命中的 5 个类全是字面冲突）。建议在报告/清单里标 `N/A`，不计入分母（否则分母 1,109，覆盖率被无意义拉低）。
2. **PyAx 232 类**（COM 自动化）：PyArxCore 现有 `PyAcadDatabase` 仅 3 个成员（809B 雏形）、与 PyRx 版 0 个共同成员。要 232 类需引入 AcAx 与 COM 类型库，工作量大于 P1+P2 总和。建议单独立项，不进本轮对齐。
3. **PyGs 6 / PyPl 27 / PySm 32**：属 AutoCAD 正式 API，建议纳入对齐范围（按 P3 排期）。这三块不是"少几个函数"，是从零建子模块，`PyPl` 还牵涉打印配置/DSD 数据。

已排除：`wx`（`pyrx/utils/wx.py` 225B、PySamples/wxPython）。

---

## 8 明确不算缺陷的部分

- PyArxCore 独有 103 个类（`AcGiContext`/`AcGiMaterialTraits`/`AcGiPolyline`/`AcGiShadeParameters`/`PyUt*` 等）是自研绘图内核内部件，不需要回退改名
- 908 个 PyArx 独有成员多为重载与便利方法（`PyDbDatabase` 多出 44 个，如返回原生 ARX 类型的 `addAcDbObject`、直取符号表的 `getBlockTable`/`getDimStyleTable`）
- PyArx 的 80 个 aced\* 函数中 48 个 PyRx 本身未导出

---

## 9 本次未做的验证（据实记录）

- **纯静态**：未编译、未在 CAD 内 `import PyArxCore`；成员名按"注册字符串"比对，**不校验重载签名、参数默认值、异常策略**
- **未做行为级对比**：例如 `ObjectId` 的相等/哈希语义、`Transaction` 嵌套规则
- aced 映射 32 条中存在 docstring 邻近噪声（约 5–8 条需人工复核，见 `aced_mapping.csv`）
- PyRx 的 15 个纯 Python 层模块（`db/block.py`、`ed/prompt.py`、`ap/utils.py`、`utils/decorators.py` 等）只统计了类/函数数量（`py_layer.csv`），未逐函数比对；若要对齐"PyRx 开箱可用的高层 API"，这部分需要在 PyArx 侧新建同名 Python 包

---

## 10 附：明细 CSV 说明（`docs_notes/api_parity/`）

| 文件 | 行数 | 列 |
|---|---|---|
| `classes.csv` | 231 | module, pyrx_class, pyarx_class, name_aligned, ac_name, pyrx_base, pyarx_base, base_match, pyrx_members, pyarx_members, missing, extra, pyarx_file |
| `missing_classes.csv` | 313 | module, class, kind(enum/class), base, pyrx_members, note |
| `missing_members.csv` | 2,374 | module, pyrx_class, pyarx_class, member, kind |
| `extra_members.csv` | 908 | 同上（PyArx 多出的成员） |
| `name_mismatch.csv` | 202 | module, pyrx_name, pyarx_registered_name, pyrx_members, how（匹配方式：alias/prefix/short） |
| `wrong_module.csv` | 19 | module, pyrx_class, pyarx_class, actual_module（同名但挂错子模块） |
| `pyarx_only_classes.csv` | 103 | pyarx_class, base, members, module, file（PyArx 独有类） |
| `module_funcs.csv` | 9 | module, func, status（1 present / 8 missing） |
| `aced_mapping.csv` | 80 | pyarx_func, pyrx_class, pyrx_method, status |
| `py_layer.csv` | 15 | pyrx_module, scope, functions, classes |
| `module_inventory.csv` | 14 | module, scope, pyrx_classes, matched_classes, class_pct, pyrx_members, aligned_members, member_pct, pyarx_submodule, found_in, note（14 个模块全量口径） |
| `extra_module_matches.csv` | 7 | module, pyrx_class, pyarx_class, found_module, shared_members, verdict（8 个未覆盖模块的 7 个名义命中逐条定性） |

匹配口径（脚本内）：先按注册名字面量建索引 → 显式 ALIAS 表 → `Py/Ac+模块标签+类名` → `Py+类名` → 裸名 → 反向短名索引，**仅接受同子模块命中**；跨子模块只记 `note` 并计入缺失（避免 `PyAp.LayerFilter↔PyDbLayerFilter` 这类假阳性）。协议成员（`__reduce__` 等）不计缺口；`PyWrapper_BIND` 宏挂载的 `desc/cast/className/appName/dxfName` 反补为已存在；`py::init` 记为 `__init__`。

---

## 11 无头实测复核（2026-11-17，accoreconsole U.61）

用 AutoCAD 2024 核心控制台（`ACAD\accoreconsole.exe`）+ `PyArxEngine.crx` 真正加载 PyArxCore 后跑 `tools/check_pyrx_compat.py`，用实测反向修正生成器，共四轮：

| 轮次 | 结果 | 修正动作 |
|---|---|---|
| 1 | 通过 52 / 失败 12 | 暴露 Ed.Editor/EditorReactor、Ed.Core.alert 等全缺 |
| 2 | 通过 53 / 失败 9 | 生成器放弃 `aced_mapping.status`（其 48 条「PyRx 亦无」被实测证伪），改按 PyRx pyi 的 Core 成员表生成，源名 4 候选 |
| 3 | 通过 53 / 失败 9 | 定位 `Db.Core` 为空：`PyDb.pyi` **无 `class Core`**，`db_core_names` 天然为空 |
| 4 | **通过 55 / 失败 0** | 候选补 `acdb` 前缀 + `cmdS→acedCmd` 覆盖；自检删掉两处错误预期 |

三处关键纠正（审计口径修正）：

1. **`Db.Core` 不是 PyRx 接口**。`PyDb.pyi` 只有模块级 `def curDb(...)`，没有 `Core` 类；原 `aced_mapping` 推出的 `Db.Core.*` 是臆造。PyArx 侧 `Db.curDb` 已存在，无需转发。
2. **`angToF/angToS` 属于 PyArx 独有**。PyRx `Ed.Core` 无这两个成员；PyArx 的实现在 `acdbAngToF/acdbAngToS`（`acdb` 前缀，非 `aced`），属额外能力，不计缺口。
3. **`aced_mapping.csv` 的 status 列不可用**。48 条判为「PyRx 亦无」的函数（含 `acedAlert`/`acedGetVar`/`acedTrans`/`acedArxLoad`）在 PyRx `Ed.Core` 中真实存在，生成器已彻底绕过该列。

实测确认的 PyArx 导出面（`tools/probe_api.py`）：`Core (22)`、`Ed (152)`、`Ed.Core (44)`、`Db (344)`、`Db.Core`（空，符合预期）。

### 仍未实现（已定性，非命名问题）

| 接口 | 状态 |
|---|---|
| `Ed.Editor` / `Ed.EditorReactor` | PyRx 的编辑器单例类，PyArx 完全无对应类；`Ed` 侧只提供模块级 `aced*` 函数。属 P2 待建类 |
| `Ed.Core.cmdUndefine` | PyArx 无 `acedCmdUndefine` 类实现 |
| `Ed.Core.coordFromPixelToWorld` | PyArx 只有反向的 `acedCurVportPixelToDisplay` |
| `Ed.Core.cmdS` | 已用语义最近的 `acedCmd` 映射（PyRx `cmdS` 走字符串命令，`acedCmd` 走 resbuf，签名不等价，调用方需注意） |

无头驱动要点（可复现）：`accoreconsole` 只认 `.crx`（post-build 从 dll 复制）；`PATH` 必须含 `X64\Release` 与 `ACAD`，`PYTHONHOME=X64\Release`；`.scr` 先 `SECURELOAD 0` 再 `arxload`，`PYRUN` 下一行给脚本路径。
---

## 12 P1 基础类型落地（2026-11-17，无头实测 74 项全过）

新增两个绑定单元（都在 `PyArxBinder.cpp` 里挂载，异常隔离不影响既有绑定）：

| 文件 | 内容 |
|---|---|
| `PyArxCore/PyDbValueTypes.h/.cpp` | `Db.Extents` / `Db.Extents2d` / `Db.ObjectIdArray` / `Db.IdPair` |
| `PyArxCore/PyEdEditor.h/.cpp` | `Ed.Editor`（静态转发） / `Ed.EditorReactor`（骨架） / `Ed.acedCoordFromPixelToWorld` |

### 值类型实现口径

- **Extents / Extents2d** 复用项目里早已写好但从未注册的 `PyDbExtents3d` / `PyDbExtents2d`（`PyDbExtents.h`），补齐 PyRx 侧的 `addPoints / coords / contains / intersectsWith / midPoint / isEmpty`；`operator==` 因 `AcDbExtents::operator==` 未从库导出，改为按角点比较（同时修掉了原先的链接期隐患）。
- **ObjectIdArray** 不注册类，直接对到内置 `list`。原因：`PyArrayCaster.h` 早已给 `AcDbObjectIdArray` 做了 `type_caster` 全特化（`PyArrayCaster.h:49-78`，Python 侧 = oldId 的 list，其 `load` 里就在执行 `value = AcDbObjectIdArray()`）。我最初又对该类型 `py::class_<AcDbObjectIdArray>` 注册一次，于是 `class_` 的 `type_caster_base` 与自定义 caster 的 `PYBIND11_TYPE_CASTER` 存储在同一类型上冲突——这条冲突 `PyArrayCaster.h:83-85` 自己就写了（`AcGePoint3d` 因此刻意不叠 sequence ctor）。表现为 native 层无异常直接卡死，Python 侧拿不到 traceback。改回既有 caster 后同一环境复测 74 项全过，说明 `AcArray` 默认构造本身没问题。
- **IdPair** ARX 无对应类型（PyRx 自造），实现为 `first/second` 两枚 `AcDbObjectId`。

### Ed.Editor

PyRx 的 `Ed.Editor` 有 65 个成员，其能力在 PyArx 侧已由 `Ed` 模块的 `aced*` / `pyed*` 函数覆盖，因此**不做重新实现，只做 staticmethod 转发**（`Ed.Editor.getPoint()` 与 `Ed.Editor().getPoint()` 都可用）。已转发 35 个成员：`activeViewportId / curViewportObjectId / entSel / nEntSelP(Ex) / getAngle / getCorner / getDist / getDouble / getInput / getInteger / getKword / getPoint / getReal / getString / initGet / getCurrentUCS / setCurrentUCS / getCurrentSelectionSet / regen / writeMessage / select*（13 个）/ ssget / ssgetkw / className`。

未覆盖：`zoom / zoomExtents / zoomWindow / zoomCenter`（需 resbuf 命令链，另建）、`traceBoundary`、`getViewportNumber`、`setAllowDuplicateSelection / duplicateSelectionsAllowed`、`ucsXDir / ucsYDir / ucsNormalVector`（从 UCS 矩阵推导，待确认 PyArx `acedGetCurrentUCS` 返回语义）。

`Ed.EditorReactor` 只提供可继承的类骨架 + `addReactor/removeReactor` 空实现；PyArx 尚无 CDB 反射器事件源，**覆写的方法不会被回调**，类文档字符串已写明。

### acedCoordFromPixelToWorld

ARX 2024 里它是 MFC 层 API（`core_rxmfcapi.h:370`，入参 `CPoint`），项目已开 `UseOfMfc=Dynamic`，因此直接绑定并支持 4 种入参：`(tuple) / (x,y) / (windnum,tuple) / (windnum,x,y)`。无头环境无图形，本轮只验证了存在性。

### Ed.Core.cmdUndefine —— 定案缺口

PyRx 的实现是 `acedCmdUndefine`（`EdCore.cpp:469`），但 **ARX 2024 SDK 的头文件里已不存在该符号**（全 inc 目录检索无 `acedCmdUndefine`）。PyArx 无法等价实现，保留为缺口而不是造一个假实现。

### 27 处基类补齐 —— 未做，原因已查明

`classes.csv` 里 200+ 行「基类不一致」绝大多数是**命名差异**（`PyDb.Dimension ↔ PyDbDimension` 同物异名），真正的语义差异在十几处。逐条核对后确认：这些包装类当前**全部直接派生 `PyDbObject` / `PyDbEntity`**（如 `class PyDbLayout : public PyDbObject`），而目标基类（`PyDbPlotSettings` / `PyDbUnderlayDefinition` / `PyDbRasterImage`）是**平行的包装类**，pybind11 的 `class_<D,B>` 要求 B 是 D 的 C++ 基类，直接改模板参数会 static_assert 失败。

正确的做法是改包装类继承链（`PyDbLayout : PyDbPlotSettings : PyDbObject`），并同步 `impObj()` 的返回类型与方法转发——这是结构性重构，牵动 Layout/Pdf/RasterImage/Wipeout/Filter 一族以及绑定的注册顺序（基类必须先注册，现有顺序里 `bind_PyDbLayout` 在 `bind_PyDbPlotSettings` 之前）。本轮**没有**做这个改动，也没有用「伪基类」蒙混，留作 P2 独立任务。
---

## §13 枚举对齐（本轮）

对照口径：PyRx 桩里的 `class Xxx(_BoostPythonEnum)` 及其成员行 `name: ClassVar[Self]  # 值`；PyArx 侧取 AutoCAD 内运行时 `__members__` 快照（`tools\probe_enum_dump.py` 产出 `log\enum_dump.json`）。比对脚本 `tools\audit_enums.py`（分模块统计）与 `tools\audit_enums2.py`（区分「别名可补」与「需新建」），明细 `docs_notes\api_parity\missing_enums.csv`、`missing_enum_members.csv`。

### 修复前

PyRx 222 个枚举类型 / 2,533 个成员；PyArx 79 个类型。缺 186 个类型（2 个只是别名：`Db.ColorMethod`、`Db.TransparencyMethod` 已存在于 Gi），已有枚举里缺 10 个成员，**成员值 0 处不一致**。

### 修复内容

1. **别名 2 处**：`Db.ColorMethod = Gi.AcCmColorMethod`、`Db.TransparencyMethod = Gi.AcCmTransparencyMethod`（同一对象，保证将来 Db 侧函数的返回值能与 PyRx 写法比较）。
2. **缺失成员 10 处**：`Db.ErrorStatus.Ok`、`Db.OpenMode.ForRead/ForWrite/ForNotify`、`Ed.DragStatus.eNormal/eOk`、`Ed.PromptStatus.eOk/kNormal`、`Gi.AcGiRegenType.kAcGiShadedDisplay`、`Gi.AcGiShadowFlags.kNoDrawFlags`。全部是 PyRx 的**同义名**（值与已有成员相同），故直接加上去而不是改值。
3. **新枚举 167 个**：生成器 `tools\gen_extra_enums.py` 读 PyRx 桩，产出 `PyArxCore\PyArxExtraEnums.h/.cpp`（Db 141 / Ge 12 / Ap 5 / Gi 4 / Ed 3 / Rx 2），值全部取自桩注释的真实数值。挂载在 `PyArxBinder.cpp` 末尾，异常隔离。
   - `py::enum_` 需要真实的 C++ enum 类型做标识，而这些名字只是「PyRx 命名常量表」，故用 `template <int N> struct Tag { enum type : int { }; }` 给每个枚举造一个 tag 类型。
   - 该 cpp 需要 `/bigobj`（169 个模板实例触发 C1128）。
4. **基线机制**：生成器不拿运行时快照判断「已存在」（否则生成物自己会污染基线、导致上次注册成功过的枚举被跳过），改读固定清单 `docs_notes\api_parity\enum_baseline.txt`（手工绑定的 106 个类型）。

### 修复后（无头实测，`log\probe_enum_dump.txt`）

| 模块 | PyRx | PyArx | 缺类型 |
|---|---|---|---|
| Ap | 5 | 5 | 0 |
| Db | 161 | 161 | 0 |
| Ed | 8 | 8 | 0 |
| Ge | 12 | 12 | 0 |
| Gi | 17 | 60 | 0（PyArx 命名更细，多出 43 个细分枚举） |
| Rx | 2 | 2 | 0 |
| Pl | 13 | 0 | 13 |
| Sm | 3 | 0 | 3 |
| Gs | 1 | 0 | 1 |

合计 222 / 248，缺 45 → **剩 17**（全是 Pl/Sm/Gs 三个未绑定模块）；成员 2,533，缺 **104**（= Pl 89 + Sm 13 + Gs 2，即全部落在未绑定模块）；**值不一致 0**。全量自检 80 项 / 0 失败。

实测抽查（`log\probe_enum_keys.txt`）：`Db.ColorMethod is Gi.AcCmColorMethod` → True；`Db.CellAlignment.kMiddleCenter.value` → 5；`Db.OpenMode.ForRead.value` → 0（与 PyRx 桩一致）；`Db.ErrorStatus.Ok` → 0；`Ed.PromptStatus.eOk` → 5100；`Gi.AcGiRegenType.kAcGiShadedDisplay` → 4。

### 尚未处理

- **Pl / Sm / Gs**：PyArx 根本没有这三个子模块（PlotSettings / SheetSet / AcGs 全未绑），补枚举的前提是先有模块，属结构性任务。
- **模块级枚举值导出**：PyRx 除注册枚举类，还把成员导出成模块级名字（`PyDb.eNumberOfCopiesNotSupported`、`Db.kByBlock`），逐文件统计共 **2,399 条**（Db 1,876 / Gi 100 / Ed 96 / Ge 147 / Ap 54 / Pl 89 / Rx 22 / Sm 13 / Gs 2）。PyArx 目前把一部分收在 `PyArxCore.Enum` 子模块（PyRx 无此子模块），是否照 PyRx 全导出待定。