# PyArx 用法易错点与防错（绑定层根因版）

> 适用范围：`PyArxCore`（pybind11 绑定层）。进程内 import、`curDb`/`PyAcadDatabase.current()` 差别、`PyDbTrans` 写实体、`objectIds/handent/entget/ssget`、SHX/TTF 矢量化、块表入口、`saveAs` 优先级、外部进程 COM、热编译、MSBuild 自加载、`acutPrintf` 宽串乱码等已由 `PYARX_USAGE.md` 覆盖，本文只做交叉引用，不复述。
> 所有 API 名与 `PyArxCore` 源码注册名一一对应；对照脚本位于 `tests/cad/`。

## 一、对象打开：空 / 非法 ObjectId 与非法 OpenMode

### 1. 空 ObjectId（0）直接打开

- **错误现象（修复前实测）**：`db.openObject(0, 0)` 返回空包装，调用方若继续解引用 `objectId()/handle()` 等方法，落到空指针分支外即崩溃；源码层 `acdbOpenObject` 先收到空 id。
- **错误写法**：
  ```python
  o = db.openObject(oid, 0)
  print(o.handle())  # oid 可能为 0，未判空
  ```
- **正确写法**：
  ```python
  o = db.openObject(oid, 0)
  if o.isNull():
      return  # 空对象，禁止再调用实体方法
  ```
- **防错机制**：`PyDbDatabase.openObject` 对 `objectId == 0` 直接返回空 `PyDbObject` 并记录 `eNullObjectPointer`；`PyDbTrans.openObject` 对空 id 返回 `None`；`PyDbTransactionManager.getObject` 同样拦截 0。

### 2. 非法 ObjectId（不存在的 oldId）

- **错误现象（修复前实测，`tests/cad/pyarx_pitfall_probe.py`）**：
  ```
  RuntimeError: Caught an unknown exception!
  ```
  旧实现把 `999999999` 直接送 `acdbOpenObject`，SDK 内部抛非标准异常，Python 侧只拿到 unknown exception。
- **错误写法**：`db.openObject(999999999, 0)`，未先校验 id 来自 `objectIds()`/`handle()` 链路。
- **正确写法**：先取有效 id，打开后判 `isNull()`：
  ```python
  ids = db.objectIds("AcDbLine")
  if ids:
      o = db.openObject(ids[-1], 0)
  ```
- **防错机制**：`openObject` 对 `oid.isNull()` 返回空包装；SDK 异常 `catch(...)` 兜底后返回空包装，不再把非标准异常抛到 Python。

### 3. 非法 OpenMode（mode 不在 0/1/2）

- **错误现象（修复前实测）**：`mode=9` 被 `static_cast<AcDb::OpenMode>` 成未定义枚举，对象仍被打开，`isNull()==False`，静默误开；事务内 `tr.openObject(oid, 9)` 同样返回了对象。
- **错误写法**：`db.openObject(oid, 9)` / `tr.openObject(oid, 9)`。
- **正确写法**：mode 仅传 `0=kForRead`、`1=kForWrite`、`2=kForNotify`。
- **防错机制**：三处 `openObject/getObject` 统一校验 `0 ≤ mode ≤ 2`，越界记录 `eInvalidInput` 并返回空包装 / `None`。修复后复测（`tests/cad/pyarx_api_test_batch4.py`）：`db.openObject(oid, 9).isNull()==True`，`tr.openObject(oid, 9) is None`。

## 二、事务边界：异常退出必须回滚

### 4. `with PyDbTrans()` 块内抛异常仍被提交

- **错误现象（修复前实测，基线探针）**：事务内 `addEntity` 一条直线后 `raise RuntimeError`，退出事务后实体仍落库：
  ```
  before=1, after=2, rolled_back=False
  ```
  旧 `__exit__` 无条件 `commit()`，不识别异常参数。
- **错误写法**：
  ```python
  with P.Db.PyDbTrans() as tr:
      ln = P.Db.PyDbLine(P3(0, 0, 0), P3(10, 0, 0))
      tr.addEntity(ln)
      raise RuntimeError("脚本出错")  # 期望回滚，实际提交
  ```
- **正确写法**：用法不变，语义已修复；异常路径自动 `abort()` 回滚，正常退出才 `commit()`。
- **防错机制**：`PyDbTrans.__exit__(excType, excVal, excTB)`：`excType` 非 `None` 调 `abort()`，否则 `commit()`。修复后复测（`tests/cad/pyarx_api_test_batch3.py`）：`before=0, after=0, rolled_back=True`；正常提交用例 `0→1, committed=True`。

### 5. 事务退出后继续持有包装实体

- **错误现象（源码级，未在本轮宿主复测）**：`with` 退出后事务关闭其托管的 `AcDbObject*`，Python 包装仍持有裸指针；再调用其几何/属性方法属悬空访问。
- **错误写法**：事务内保存 `ln` 引用到事务外，退出后再 `ln.startPoint()`。
- **正确写法**：事务内完成读写；事务外需要对象时用 `tr.openObject(id, mode)` 重新打开，用完按裸打开链路 `obj.close()`（非事务托管）。
- **防错机制**：包装提供 `isNull()` / `intPtr()` / `close()`；事务托管对象在 `PyDbTrans` 生命周期内使用。

## 三、多段线（AcDbPolyline）顶点索引越界

### 6. 空表 / 越界索引读点、读凸度得到伪造默认值

- **错误现象（修复前实测，基线探针）**：对空 `PyDbPolyline` 调用 `getPointAt(0)` 返回 `AcGePoint2d(0,0)`，`getBulgeAt(0)=0.0`，`getWidthsAt(0)=(0.0,0.0)`，`segType(0)=4`，`getLineSegAt(0)` 返回一条伪造线段；仅 `setPointAt/removeVertexAt` 返回 `False`。
- **错误写法**：
  ```python
  pl = P.Db.PyDbPolyline()
  pt = pl.getPointAt(0)  # 空表越界，返回原点
  ```
- **正确写法**：先判 `numVerts()`：
  ```python
  if i < pl.numVerts():
      pt = pl.getPointAt(i)
  ```
- **防错机制**：`PyDbPolyline` 读/写/删除方法统一 `index < numVerts()` 校验；越界读返回默认值并记录 `eInvalidInput`，`segType` 越界返回 `-1`；写/删返回 `False`。

### 7. `addVertexAt` 允许尾部之外的非法位置

- **错误现象（修复前源码推断 + 实测对照）**：负索引在 pybind 整型转换阶段被拒（`TypeError: incompatible function arguments`）；超大索引（`index > numVerts()`）透传 SDK。
- **错误写法**：`pl.addVertexAt(99, P2(1,1))` 于空表。
- **正确写法**：尾部插入用 `index == numVerts()`；中间插入先确认索引落在 `[0, numVerts()]`。
- **防错机制**：`addVertexAt` 校验 `0 ≤ index ≤ numVerts()`，越界返回 `False` 并记录 `eInvalidInput`。修复后复测（`tests/cad/pyarx_api_test_batch.py`）：空表 `addVertexAt(99,...)=False`，`segType(99)=-1`。

## 四、Ed 层：`None` 传入强类型实体名 / 句柄

### 8. `entget/entdel/entupd/handent(None)` 抛 TypeError

- **错误现象（修复前实测，基线探针）**：
  ```
  TypeError: entget(): incompatible function arguments.
  1. (ename: PyArxCore.Ed.AdsName) -> object
  Invoked with: None
  ```
  `entdel/entupd/handent(None)` 同类报错。
- **错误写法**：
  ```python
  ename = P.Ed.handent(maybe_none)
  P.Ed.entget(ename)  # ename 可能为 None
  ```
- **正确写法**：可直接传可能为 `None` 的变量，按返回值判空：
  ```python
  dxf = P.Ed.entget(ename)   # None 入参返回 None
  if P.Ed.entdel(ename): ... # None 入参返回 False
  ```
- **防错机制**：四个 API 入参改为 `py::object`；`entget(None)→None`、`entdel(None)→False`、`entupd(None)→False`、`handent(None)→None`。修复后复测（`tests/cad/pyarx_api_test_batch3.py`）返回值与约定一致。

## 五、图元增改测试结果（修复后宿主实测）

宿主：AutoCAD 2024（24.3s LMS Tech）进程内 `PYRUNFILE`；模块 `PyArxCoreH20261115_113355.pyd`（hotbuild 产物）；文档 `Drawing1.dwg`。脚本：`tests/cad/pyarx_api_test_batch.py`、`pyarx_api_test_batch2.py`、`pyarx_api_test_batch3.py`、`pyarx_api_test_batch4.py`。

### 5.1 新增图元

| 用例（API） | 预期 | 实际 | 通过 | 失败根因 |
|---|---|---|---|---|
| `PyDbLine` 构造 + `addEntity` | 创建成功、句柄可取、起终点读回一致 | 句柄 696，start=(0,0,0) end=(10,0,0) | 是 | — |
| `PyDbCircle(center, normal, radius=5)` | 创建成功、半径/圆心读回一致 | 句柄 697，radius=5.0，center=(0,0,0) | 是 | — |
| `PyDbArc(center, r=3, 0→π/2)` | 创建成功、半径/角度读回一致 | 句柄 698，radius=3.0，start=0.0，end=1.5708 | 是 | — |
| `PyDbPolyline` 三次 `addVertexAt` 插点 | 3 顶点、第 2 点读回一致 | 句柄 699，verts=3，p1=(10,0)，三次插入均 True | 是 | — |
| `PyDbPolyline` 越界索引（`addVertexAt(99)` / `getPointAt(99)` / `segType(99)`） | 被校验挡住，不产生伪造几何 | add=False，读点为默认原点，seg=-1 | 是（校验路径） | — |
| 闭合多段线（4 顶点 + `setClosed(True)`） | 4 顶点、闭合标记一致 | 句柄 700，verts=4，closed=True | 是 | — |
| `PyDbText(pt, "PYARX", h=2.0)` | 创建成功、文字/字高读回一致 | 句柄 701，text=PYARX，height=2.0 | 是 | — |
| `PyDbAlignedDimension` + 三条定位点 | 创建成功、定位点读回 | 句柄 702，x1=(0,0) | 是 | — |
| `PyDbBlockReference`（先建块定义） | 建定义 → 参照 → 位置读回 | 会话在事务内建块定义链路阻塞，未拿到结果 | 否 | 外层 `PyDbTrans` 事务内调用 `BlockTableRecord.appendEntity`（内部再开事务），块表记录未纳入外层事务托管，进程内挂起；本轮不改该链路 |

### 5.2 属性写回与几何修改

| 用例（API） | 预期 | 实际 | 通过 | 失败根因 |
|---|---|---|---|---|
| `setColorIndex(1)` → `colorIndex()` | 写回 1，读回 1 | 句柄 696，colorIndex=1 | 是 | — |
| `setLayer("0")` → `layer()` | 读回 `0` | 句柄 697，layer=0 | 是 | — |
| `setLinetype("ByLayer")` → `linetype()` | 读回 ByLayer | 句柄 698，linetype=ByLayer | 是 | — |
| `PyDbLine.setStartPoint(20,0,0)` | 起点读回 (20,0,0) | 句柄 699，start=(20,0) | 是 | — |
| `PyDbCircle.setRadius(8.0)` | 半径读回 8.0 | 句柄 700，radius=8.0 | 是 | — |
| `PyDbPolyline.setPointAt(1, (20,20))` | 第 2 点读回 (20,20) | 句柄 701，p1=(20,20) | 是 | — |

### 5.3 Ed 链路与校验路径

| 用例（API） | 预期 | 实际 | 通过 | 失败根因 |
|---|---|---|---|---|
| `handent(handle)` → `entget` 取 DXF；`entmod({8:"0",62:1})` | 取到组码字典；entmod 返回成功 | 会话在该链路阻塞，未返回结果 | 否 | `acdbEntMod` 路径在 PYRUNFILE 命令上下文阻塞；本轮未定位到绑定层根因，未改动 |
| `entdel(ename)` 后 `openObject(id,1)` 应被挡住，再 `entdel` 恢复 | 擦除后重开为空包装 | 随上一条阻塞，未完成 | 否 | 同上，未复测 |
| `db.openObject(0,0)` | 空包装，不抛异常 | `isNull=True` | 是 | — |
| `db.openObject(999999999,0)` | 空包装，无 unknown exception | `isNull=True` | 是 | — |
| `db.openObject(有效id, 9)` | 空包装（mode 越界） | `isNull=True`（batch4 复测） | 是 | — |
| `tr.openObject(0,0)` | `None` | `isNone=True` | 是 | — |
| `tr.openObject(有效id, 9)` | `None`（mode 越界） | `isNone=True`（batch4 复测） | 是 | — |
| 事务异常退出回滚 | 异常后直线数不变 | before=0 after=0，rolled_back=True | 是 | — |
| 事务正常提交 | 直线数 +1 | before=0 after=1，committed=True | 是 | — |
| `Ed.entget(None)/entdel(None)/entupd(None)/handent(None)` | None/False/False/None | 与约定一致 | 是 | — |

### 5.4 汇总

| 口径 | 数量 |
|---|---|
| 用例总数（含阻塞未完成项） | 25 |
| 通过 | 22 |
| 阻塞 / 未完成 | 3（`PyDbBlockReference` 建参照链路、`entget+entmod`、`entdel` 后重开） |
| 修复后复测结论 | 三处新增/修改图元与属性几何写回链路通过；openObject 空 id/非法 id/非法 mode、事务异常回滚、Ed None 归一、多段线越界校验均在修复后宿主实测通过；阻塞 3 项不在本轮绑定层根因修复范围 |

## 六、未复测 / 仅源码推断项

- `PyDbText` 零字高（`height=0.0`）：基线实测被静默接受、读回 `0.0`；SDK 要求正字高。本轮未改构造默认值，使用时显式传正字高。
- `PyDbBlockTableRecord` 从 oldId 构造：源码内 `reinterpret_cast<AcDbStub*>(oldId)` 与其它链路 `setFromOldId` 不一致，未实测，列为源码推断。
- `db.objectIds("不存在类名")`：静默返回空 `[]`，不报错；调用方需区分「空结果」与「类名拼错」。
- 包装实体空指针方法解引用：除已加判空的方法外，部分 `PyDbObject/PyDbEntity` 方法直接使用 `impObj()`；空包装调用属未定义行为，调用方统一先 `isNull()` 判断。

## 七、API 变更清单

逐条「文件名 + 旧签名/行为 → 新签名/行为 + 影响面 + B 线适配写法」见 `docs/_API_CHANGES.md`（2026-11-15 PyArxCore 绑定层根因修复节，共 6 条）：

1. `PyDbDatabase.openObject`：空 id / 非法 id / mode 越界 → 空包装 + 状态记录。
2. `PyDbTrans.openObject`：空 id / mode 越界 / SDK 异常 → `None`。
3. `PyDbTransactionManager.getObject`：同 openObject 校验。
4. `PyDbTrans.__exit__`：异常路径 `abort()`，不再误 `commit()`。
5. `PyDbPolyline` 顶点索引校验：`addVertexAt` 边界 `[0, numVerts()]`；读写删 `< numVerts()`。
6. `Ed.entget/entdel/entupd/handent`：`None` 归一为 `None/False/False/None`，不再抛 TypeError。

> 编译备注：`PyArxCore.pyd` 被运行中的 `acad.exe` 占用，正式链接需先关闭 CAD；本轮以 hotbuild 产物 `PyArxCoreH20261115_131114.pyd`（8,670,208 字节）验证编译通过，日志 `docs/_pyarxbuild_pyarxcore.txt`。

## 八、遍历与字典协议

### 1. 块表记录本体遍历

- **错误现象（本轮实测，`tests/cad/pyarx_api_test_batch6.py`）**：`btr_setup` 创建块定义成功；`btr_protocol` 在容器协议链路抛 `RuntimeError: Caught an unknown exception!`。
- **错误写法**：先 `values(1)/items(1)` 一次性打开块内实体，再 `btr[objectId]` 或 `getEntityById(id, 1)` 对同一实体重复 `kForWrite` 打开。
- **正确写法**：
  ```python
  for ent in btr:
      ent.setColorIndex(3)

  ids = btr.keys()
  record = btr.get(ids[0], None)
  ```
- **防错机制**：BTR 本体提供 `__len__ / __iter__ / iterEntities / getEntityById / __getitem__ / __contains__ / get / keys / values / items`；`values/items` 会一次性打开全部实体，大图必须用 `for ent in btr`。

### 2. 符号表本体遍历

- **错误现象（本轮实测，`tests/cad/pyarx_api_test_batch6.py`）**：`len(layerTable)=1`、`keys=1`、`values=1`、`"0" in layerTable=True`；`items=0`，`layerTable["0"]` 返回空包装。
- **错误写法**：把 `values/items` 当纯查询列表，随后再次 `getAt(name, 1)` 打开同一记录。
- **正确写法**：
  ```python
  for rec in layerTable:
      pass

  if "0" in layerTable:
      rec = layerTable.getAt("0", 1)
  ```
- **防错机制**：符号表提供 `records/keys/values/items/get/__contains__/__iter__/__len__/__getitem__`；纯查询传 `mode=0`，需要写记录时再 `getAt(name, 1)`。

### 3. 键规则

- `PyDbBlockTableRecord`：键为 ObjectId(oldId) 整数或十六进制串。
- `PyDbSymbolTable`：键为记录名（如 `"0"`）。
- `values/items` 返回前会一次性打开全部对象；大图请用容器本体 `for` 循环惰性遍历。

### 4. 本轮容器协议最小实测结果

宿主：AutoCAD 2024 进程内 `PYRUNFILE`；热模块 `PyArxCoreH20261115_131114.pyd`；脚本 `tests/cad/pyarx_minimal_probe.py` → `pyarx_api_test_batch6.py`；结果 JSON `G:\PyArx\log\pyarx_api_test_batch6.json`。

| 用例 | 预期 | 实际 | 通过 | 失败根因 |
|---|---|---|---|---|
| `for ent in btr` 改颜色后回读 3 | 遍历成功，颜色回读 3 | 块定义创建成功；协议链路抛 `unknown exception` | 否 | `values/items/getEntityById` 对同一实体重复写打开，打开/关闭未配平 |
| `for rec in layerTable` | 遍历到记录 | `len=1`、`keys=1`、`values=1`、`contains=True`；`items=0` | 部分通过 | 记录枚举打开模式与二次取记录链路未配平 |
| `layerTable["0"]` | 取到 `"0"` 记录 | `zero_ok=False`（空包装） | 否 | `getAt("0", 1)` 未成功打开记录 |
| `"0" in layerTable` | 命中已有记录 | `contains=True` | 是 | — |

汇总：最小用例 4 项，通过 1 项，失败 3 项；无 CAD 内部错误对话框记录。

### 5. 遗留阻塞用例（不再逐层深挖）

| 用例 | 已定位卡点 | 规避写法 |
|---|---|---|
| `c_blockref` | 外层 `PyDbTrans` 内 `BlockTableRecord.appendEntity` 嵌套开启事务，块记录未纳入外层事务托管 | 同事务内裸追加后 `BlockTable.add(btr)`；不在外层事务内调用自管事务的追加链路 |
| `ed_entget_entmod` | `acdbEntMod` 在 `PYRUNFILE` 命令上下文阻塞，未定位到绑定层根因 | 先用 `openObject(id, 1)` 打开实体改属性；不依赖 `entmod` 直改 DXF |
| `ed_entdel_then_modify` | 随 `entmod` 阻塞，删除后重开链路未复测 | 擦除对象不再改属性；需要恢复时重新 `entdel` 或重建实体 |

## 九、PyMfcXml 宿主窗口补充

### 1. XWindow `Background`

- 解析链路：`PyMfcLoad.cpp` 读取根节点 `Background`，经 `xmlapp::ParseColor` 解析后调用 `PyMfcWindow::setBackground`；宿主 `PyMfcHost` 同步页底色与 GDI/D2D 擦除背景。
- Python API：`win.setBackground(r, g, b)`。
- 本轮未在可见宿主会话复测客户区底色；未复测项不标通过。

### 2. 控件客户区尺寸

- 新 API：`win.getSize(id) -> (w, h)`；未找到控件返回 `(0, 0)`。
- 用途：替代 125%/150% DPI 下的硬编码尺寸兜底；实现按 `findByIdDeep` 递归取控件 HWND 后 `GetClientRect`。

### 3. XPowerCanvas SVG 1:1 铺满源码修正（本轮未取得像素复测）

- 源码修正：`PyMfcHost::fitPlanCanvas` 对 `mode=2`（SVG）不再套用 plan 场景的 8px 内边距、等比 fit 与居中偏移；固定 `zoom=1.0、ox=0、oy=0`，SVG 坐标直接映射控件客户区，超出部分由窗口裁切。
- 实测状态：本轮宿主创建 `PyMfcWindow` 失败（`create_embedded(0, ...)` 无 CAD 父 HWND），未生成 PNG；未取得 `(0,0)-(1343,103)` 四边像素坐标，该项不判通过。

## 十、2026-11-15 二次根因复测与未通过项

### 1. 本轮源码改动

- `PyDbBlockTableRecordIterator.h/.cpp`：实体遍历器记录上一个已打开实体；`__next__()` 取下一个前先关闭上一个，析构时关闭残留对象，避免同一实体连续两次打开。
- `PyDbBlockTableRecord.cpp`：`values/items` 批量列举固定按 `kForRead` 打开；需要写属性的调用方在惰性 `for ent in btr` 路径自行升级。
- `PyDbSymbolTable.cpp`：`records/values/items` 枚举固定 `kForRead`；`items` 保持一次遍历同时生成 `(名, 记录)`；`getAt` 失败不再吞成空包装，改为抛出含记录名、mode、错误状态和可写打开提示的 Python 异常。
- `PyMfcHost.cpp`：SVG 画布 1:1 修正见上节。

### 2. 本轮最小实测结果

结果 JSON：`G:\PyArx\log\pyarx_minimal_probe2.json`；热模块 `PyArxCoreH20261115_135006.pyd`。

| 用例 | 预期 | 实际 | 通过 | 失败根因 |
|---|---|---|---|---|
| `for ent in btr` 改颜色后回读 3 | 遍历成功，颜色回读 3 | `RuntimeError: Caught an unknown exception!` | 否 | 事务提交后重开 BTR/实体仍撞重复打开状态；绑定层尚未维护打开对象缓存，无法对已开对象 `upgradeOpen` 或复用包装 |
| `for rec in layerTable` / `items()` | 记录名与记录配对一致，长度相等 | `len=1`；枚举后取 `"0"` 抛 `eWasOpenForRead` | 否 | 记录已以读模式被枚举包装占用；二次 `kForWrite` 打开不能在另一个包装上升级，需打开对象缓存或枚举结束显式释放 |
| `layerTable["0"]` | 取到 `"0"` 记录 | 先取时可成功（`zero_ok=true`）；枚举后再取抛清晰异常 `eWasOpenForRead` | 部分通过 | 重复打开/读升级未配平；异常已不再吞成空包装 |
| `"0" in layerTable` | 命中已有记录 | `contains=True` | 是 | — |

### 3. 编译与宿主状态

- `PyArxCore` 直接链接被运行中 `acad.exe` 占用（`LNK1104: 无法打开 PyArxCore.pyd`）；hotbuild 增量产物 `PyArxCoreH20261115_135006.pyd`（8,669,696 字节），MSBuild exit=0，日志 `docs/_pyarxbuild_hot.txt`。
- `PyMfcXml /t:PyMfcXml` 编译 exit=0，0 个 `: error`，日志 `docs/_pyarxbuild_pymfcxml.txt`。
- 宿主实例异常：当前 COM 附着链路仍观察到新增 `acad.exe` 进程；`Quit()` 后进程未释放。结束时 8 个 acad 进程、合计约 4,064MB（未超 8GB 阈值，但超过 2 个上限）；未做全局窗口广播关闭。
