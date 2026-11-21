# _API_CHANGES.md —— 2D CAD（examples/cad2d）侧登记的底层缺口

> 规则：本示例不改 C++（PyArxCore / PyArxEngine / MfcXmlCore / PyMfcXml 归 A 线）。
> 发现底层缺 API 时在此登记（API 名 + 期望签名 + 用途），示例内给最小可用 Python 兜底。

## 2026-11-15 cad2d 缺口登记

### 1. XDwgCanvas 未派发键盘事件与右键事件
- 现状：`onCanvasEvent(id, cb)` 仅回调 `down / move / up / dblclick / wheel`；
  宿主 `PyMfcDwgCanvasMouse` 只处理 WM_LBUTTONDOWN/UP/DBLCLK、WM_MBUTTONDOWN、WM_MOUSEMOVE、WM_MOUSEWHEEL，
  WM_KEYDOWN / WM_KEYUP / WM_RBUTTONUP / WM_RBUTTONUP 没有走到 Python。
- 期望签名（A 线后续补）：
  - `win.onCanvasEvent(id, cb)` 的 cb 支持 `phase in {"keydown","keyup"}`，载荷 `(phase, vKey, 0, 0)`；
  - 右键：`phase == "rightdown" / "rightup"`，与左键同构。
- 用途：CAD 标准交互里 Esc 取消命令/夹点拖动、右键回车/取消。
- Python 兜底：命令行输入 `ESC`（别名 `取消`）走 `cmds.cancel()` 并清除夹点拖动/框选；
  夹点拖动在"点中空白处松开"或重新点选其它实体时作废，不写入历史栈；
  物理 Esc 键与右键取消为已知限制（见交付结论）。

### 2. 画布控件尺寸 API 缺失（XML 自绘控件）
- 现状：`PyMfcWindow` 绑定了 `clientSize()`（整窗），没有 `getSize(id)`；
  app.fit() 读不到 cvMain 的客户区宽高，只能走 820×592 兜底。
- 期望签名：`win.getSize(id) -> (int w, int h)`。
- 用途：全图适配时按真实画布尺寸算缩放。
- Python 兜底：`try win.getSize` 失败后用 `clientSize()` 推算，再退到固定兜底值。

### 3. 表格行读取：XML `XTableView` 的 ViewRows 不可用
- 现状：`setTableRows(id, blob)`（行间 `\n`、格间 `\t`）写入正常，
  `getTableRowCount / getTableCell` 可读；但 `tableViewRows(id)` 对该控件返回 0
  （新自绘表 TableControl 视口行计数分支未命中）。
- 期望签名：`tableViewRows(id) -> int` 与真实行数据一致。
- 用途：图层表滚动/可视行计算（当前不需要滚动，非阻塞）。
- Python 兜底：图层表刷新只依赖 `setTableRows` 写入 + `getTableCell` 校验，不读 ViewRows。

### 4. SVG 场景对 pattern / clipPath / 多边形裁剪的支持未验证
- 现状：`XmlSvgScene` 已确认支持 line/circle/path/rect/text/stroke-dasharray；
  `<pattern>`、`<clipPath>`、`fill-rule` 未在本示例实测。
- 用途：HATCH 区域填充（闭合多边形内画斜线，需裁剪边界）。
- 兜底：HATCH 命令未接线；DIM 实体与渲染桩就位、命令状态机未接线，均待底层能力确认后再接。

### 5. 备注：setTableRows 分隔符
- 已确认可用签名：`win.setTableRows(id, "行1格1\t行1格2\n行2格1\t行2格2")`。
- 旧示例误用 `|` 分行，已在 Python 侧改为 `\n` / `\t`。

---

## 2026-11-15 PyArxCore 绑定层根因修复（A 线）

### 1. PyDbDatabase.openObject
- 文件：`PyArxCore/PyDbDatabase.cpp`（声明 `PyArxCore/PyDbDatabase.h`）
- 旧签名/行为：`PyDbObject openObject(uint64_t objectId, int mode)`；objectId=0 与非法 oldId 直接进 `acdbOpenObject`，非法 id 抛非标准异常（Python 侧 `RuntimeError: Caught an unknown exception!`）；mode 任意 int 被 `static_cast<AcDb::OpenMode>` 成未定义枚举。
- 新签名/行为：签名不变；objectId=0、mode∉{0,1,2}、非法 id 一律返回空 `PyDbObject`（`isNull()==True`），经 `PyWarnIfError` 记录错误状态；SDK 异常 try/catch 兜底。
- 影响面：所有 `db.openObject(...)` 调用方；返回空包装不再抛异常，调用方需判 `isNull()`。
- B 线适配：`o = db.openObject(oid, 0); if o.isNull(): return`；mode 仅传 0/1/2。

### 2. PyDbTrans.openObject
- 文件：`PyArxCore/PyDbTrans.cpp`（声明 `PyArxCore/PyDbTrans.h`）
- 旧签名/行为：`py::object openObject(AcDbObjectId id, int mode, bool openErased)`；mode 未校验；非法 id 可能抛非标准异常。
- 新签名/行为：签名不变；id 为空、mode∉{0,1,2}、SDK 异常兜底均返回 `None`，并记录错误状态。
- 影响面：事务内 `tr.openObject(...)` 调用方。
- B 线适配：`o = tr.openObject(oid, 1); if o is None: ...`；mode 仅传 0/1/2。

### 3. PyDbTransactionManager.getObject
- 文件：`PyArxCore/PyDbTransactionManager.cpp`
- 旧签名/行为：`PyDbObject getObject(uint64_t objId, int mode, bool openErased)`；objId=0、mode 越界未校验。
- 新签名/行为：签名不变；objId=0、mode∉{0,1,2} 返回空包装并记录状态。
- 影响面：`db.transactionManager().getObject(...)` 调用方。
- B 线适配：判 `isNull()`；mode 仅传 0/1/2。

### 4. PyDbTrans.__exit__
- 文件：`PyArxCore/PyDbTrans.cpp`（声明 `PyArxCore/PyDbTrans.h`）
- 旧签名/行为：`void exit(py::object, py::object, py::object)`；with 块内 Python 异常向上传播时仍无条件 `commit()`，异常路径实体被持久化。
- 新签名/行为：签名不变；`excType` 非 None 时 `abort()` 回滚，仅正常退出才 `commit()`。
- 影响面：所有 `with PyDbTrans():` 用法；异常路径语义由「误提交」改为「回滚」。
- B 线适配：无需改写法；异常内已 addEntity 的实体不再落库，属预期修复。

### 5. PyDbPolyline 顶点索引校验
- 文件：`PyArxCore/PyDbPolyline.cpp`（声明 `PyArxCore/PyDbPolyline.h`）
- 旧签名/行为：`addVertexAt(unsigned int index, ...)` 与 `getPointAt/getBulgeAt/getWidthsAt/setPointAt/setBulgeAt/setWidthsAt/removeVertexAt/segType/getLineSegAt/getArcSegAt(index)`；索引越界透传 SDK，空表读点返回伪造原点/0，`addVertexAt` 允许任意大索引。
- 新签名/行为：签名不变；`addVertexAt` 要求 `0 ≤ index ≤ numVerts()`，越界返回 `False`；读/写/删除方法要求 `index < numVerts()`，越界返回默认值/`False`/`-1` 并记录 `eInvalidInput`。
- 影响面：所有 LWPOLYLINE 顶点操作；越界不再产生伪造几何。
- B 线适配：先 `numVerts()` 判长；尾部插点用 `index == numVerts()`。

### 6. Ed 层 None 入参归一
- 文件：`PyArxCore/PyEdBinder.cpp`
- 旧签名/行为：`entget(const PyAdsName&)`、`entdel(const PyAdsName&)`、`entupd(const PyAdsName&)`、`handent(const AcString&)`；传 `None` 在 pybind 类型转换阶段抛 `TypeError: incompatible function arguments`。
- 新签名/行为：入参改为 `py::object`；`entget(None)`→`None`，`entdel(None)`→`False`，`entupd(None)`→`False`，`handent(None)`→`None`；非 None 仍按原类型转换。
- 影响面：`P.Ed.entget/entdel/entupd/handent` 调用方；None 不再抛 TypeError。
- B 线适配：可直接传可能为 None 的句柄变量，按返回值判空。

> 备注：`PyArxCore.pyd` 被运行中的 `acad.exe` 占用，正式链接需先关闭 CAD；本轮以 `tools/hotbuild` 产物 `PyArxCoreH20261115_131114.pyd` 验证编译通过。

## 2026-11-15 cad2d 外壳 UI（B 线现状 + 待补底层）

### 12. XML Label/Button 的外观属性未灌入 paint 节点
- 现状：`PyMfcLoad.cpp` 的 label/button 创建分支只 `addLabel/addButton`，
  没有把 `AppearanceOf(el)` 经 `setControlAppearance(id, ap)` 灌进宿主；新 surface 路径
  下 `findAppearance(id)` 恒为空，`drawOwnerNode` 走非 styled 分支，底板取主题
  surface / `m_btn`，文字走 `contrastText`。XML 上写的 `Background / Foreground /
  BorderStyle / TextAlign / Radius` 对这两类控件不生效。
- 表现：深色主题下只读标签仍是浅色底板；标签底色/文字色/对齐不可逐控件定制。
- 期望签名：创建 label/button 后统一 `host->setControlAppearance(id, AppearanceOf(el))`，
  与容器 backdrop 同一条外观链路。
- Python 兜底：cad2d 右栏特性/视图行、命令标签/提示一律改用 `XDwgCanvas` 自绘 SVG
  （`ui.props_panel / ui.text_bar`），配色与对齐在 Python 侧完全可控；仅 `XTextField`
  与可点的「执行」保留原生控件。

### 13. 单控件文字色/底板色无 Python 绑定
- 现状：`PyMfcHost::setLabelColor(id,r,g,b)`、`setControlAccent(...)` 在
  `PyMfcWindow` 有转发（PyMfcWrap），但 `PyMfcBind.cpp` 未 `.def` 到 Python；
  `setControlAppearance` 同样未暴露。Python 侧无法事后给单个原生标签/按钮改文字色或底板。
- 期望签名：`win.setLabelColor(id, r, g, b)`、`win.setControlAppearance(id, ap)` 绑定 Python。
- Python 兜底：同 12，需要逐控件配色的展示位全部走自绘画布。

### 14. SVG 场景只解析 6 位 / 3 位 hex 颜色，8 位 ARGB 兜底红色
- 现状：`XmlSvgScene.cpp` 颜色解析 `#%2x%2x%2x`（size≥7）/ `#%1x%1x%1x`（size≥4）；
  8 位 `#FFRRGGBB` 解析错位或失败，落到 fallback `red`（RGB 255,0,0）。
- 注意：XML 根/容器的 `Background="#FF26292D"` 由 C++ 另一链路解析，支持 8 位；
  仅 **SVG fill/stroke** 内必须用 6 位（或 3 位）hex。
- Python 兜底：cad2d 自绘 SVG 内颜色统一 6 位（见 `ui/__init__.py` token）；命令行底色
  用 `#202327`，不用 `#FF202327`。

### 15. nanosvg 不支持 pattern / clipPath / fill-rule
- 现状：`XmlSvgScene.cpp` 形状遍历只把 `NSVG_PAINT_COLOR` 与线性 / 径向渐变映射为填充；
  `NSVG_PAINT_PATTERN` 与 `clipPath` 裁剪被忽略，图案填充形无 `hasFill` 直接跳过。
- Python 兜底：HATCH 不走 SVG pattern，由 `render` Python 侧扫描线在多边形内求交，
  成对输出剖面 `<line>`；边界仅接矩形 / 闭合多段线。

## 2026-11-15 容器协议与宿主窗口补充（A 线）

### 7. 符号表容器协议绑定
- 文件：`PyArxCore/PyDbSymbolTable.cpp`
- 新签名：
  - `records(mode=1) -> list[PyDbSymbolTableRecord]`
  - `keys() -> list[str]`
  - `values(mode=1) -> list[PyDbSymbolTableRecord]`
  - `items(mode=1) -> list[tuple[str, PyDbSymbolTableRecord]]`
  - `get(name, default=None) -> PyDbSymbolTableRecord | default`
  - `__contains__(name) -> bool`
  - `__iter__` / `__len__` / `__getitem__(name)`
- 影响面：图层/线型/文字样式/标注样式等符号表遍历；纯查询传 `mode=0`，显式写操作可 `getAt(name, 1)`。
- B 线适配：`for rec in layerTable:`、`layerTable["0"]`、`"0" in layerTable`；批量取记录用 `values(0)`，大图避免 `values/items` 一次性打开全部记录。

### 8. 块表记录容器协议绑定
- 文件：`PyArxCore/PyDbBlockTableRecord.cpp`
- 新签名：`len(btr)`、`for ent in btr`、`iterEntities(mode=1, skipDeleted=True)`、`getEntityById(id, mode=1)`、`btr[objectid|hex]`、`get(key, None)`、`keys()`、`values(mode=1)`、`items(mode=1)`、`__contains__`。
- 影响面：遍历块定义内实体；`values/items` 一次性打开全部实体，大图请用 `for ent in btr`。
- B 线适配：BTR 键为 ObjectId(oldId) 或十六进制串；符号表键为记录名。

### 9. 公开迭代器类退役
- 文件：`PyArxCore/PyDbBlockTableRecordIterator.cpp`、`PyDbSymbolTableIterator.cpp`、`PyDbBlockTableIterator.cpp`、`PyDbLayerTableIterator.cpp`、`PyDbLinetypeTableIterator.cpp`、`PyDbTextStyleTableIterator.cpp`、`PyDbRegAppTableIterator.cpp`、`PyDbDimStyleTableIterator.cpp`、`PyDbViewTableIterator.cpp`、`PyDbViewportTableIterator.cpp`、`PyDbUCSTableIterator.cpp`、`PyDbAbstractViewTableIterator.cpp`、`PyDbXrecordIterator.cpp`、`PyDbObjectIterator.cpp`
- 变更：15 个公开 Python 类名解绑；C++ 类保留为内部实现。实体迭代器以私有绑定名 `_PyDbBlockTableRecordEntityIter` 保留给 `btr.__iter__`。
- 保留公开：`PyDbFilteredBlockIterator`、`PyDbCompositeFilteredBlockIterator`、`PyDbSpatialIndexIterator`、`PyDbLayerIndexIterator`、`PyDbIndexUpdateDataIterator`、`PyDbBlockChangeIterator`、`PyDbLongTransWorkSetIterator`。
- B 线适配：不再 `PyDbXxxIterator(...)`；遍历一律写在容器本体上。

### 10. PyMfcXml 窗口背景与尺寸 API
- 文件：`PyMfcXml/PyMfcWrap.h/.cpp`、`PyMfcBind.cpp`、`PyMfcHost.h/.cpp`、`PyMfcLoad.cpp`
- 新签名：
  - `Window.setBackground(r: int, g: int, b: int) -> None`
  - `Window.getSize(id: int) -> tuple[int, int]`
- 影响面：XML `XWindow Background="#FFF3F5F8"` 解析后铺宿主客户区；控件客户区尺寸替代硬编码兜底。
- B 线适配：`win.setBackground(255, 243, 248)`；`w, h = win.getSize(id)`；未找到控件返回 `(0, 0)`。

### 11. 容器枚举打开模式与重复打开护栏（2026-11-15 二次复测）

- `PyDbBlockTableRecord.values/items`：批量列举内部固定 `kForRead`；写属性应走惰性 `for ent in btr` 或显式 `getEntityById(id, 1)` 后自行升级。
- `PyDbSymbolTable.records/values/items`：批量列举内部固定 `kForRead`；`items` 一次遍历同时生成 `(name, record)`，不允许先 keys 再 values 双重打开。
- `PyDbSymbolTable.getAt(name, mode)`：打开失败不再返回空包装；抛出含记录名、mode、SDK 错误状态、“需以可写方式打开数据库或另存副本；纯查询传 0”提示的 Python 异常。
- `PyMfcHost` SVG 画布：SVG 模式固定 1:1（`zoom=1, ox=0, oy=0`），不再按 plan 场景做 8px 内边距、等比 fit 或居中偏移。
- 已知残留：绑定层尚未维护打开对象缓存；已以读模式打开的记录/实体在另一个包装上不能直接升级写，重复打开会返回 `eWasOpenForRead`。
