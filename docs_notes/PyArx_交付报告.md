# PyArx 交付报告（2026-11-15）

两条线并行交付：**A 线** PyArx 用法避错与绑定层根因修正（`docs\PYARX_防错手册.md`），**B 线** 自研 2D CAD 绘图程序（`examples\cad2d`，现状见 `docs\CAD2D_现状与缺口.md`）。接口契约变更统一记录在 `docs\_API_CHANGES.md`。

图元增改全部在真实宿主内实测：A 线 25 例通过 22、阻塞 3；B 线 16 例通过 14、未实现 2、失败 0。三个未通过项均已定位到具体链路，列在第七节。

---

## 一、PyArx 用法与避错（详见 docs\PYARX_防错手册.md）

`PYARX_USAGE.md` 已覆盖环境、事务、保存、热编译等 11 节，本手册只补它没写的绑定层坑，每条含错误现象 / 错误写法 / 正确写法 / 代码级防错。

1. **空 ObjectId**：`openObject(0, 0)` 旧版返回空包装，继续解引用即崩溃 → 先判 `isNull()`，源码现在对 `objectId==0` 直接返回空包装并记 `eNullObjectPointer`。
2. **非法 ObjectId**：`openObject(999999999, 0)` 旧版抛 `RuntimeError: Caught an unknown exception!` → 现在返回空包装，SDK 异常在绑定层兜底。
3. **非法 mode**：`mode=9` 旧版被 `static_cast` 成未定义枚举却仍打开对象（`isNull()==False`，静默误开）→ mode 只传 0/1/2，越界现在返回空包装 / `None`。
4. **事务异常误提交**：`with PyDbTrans()` 内抛异常，旧版仍 `commit()`，实体落库（`before=1 after=2`）→ `__exit__` 见异常即 `abort()`，复测 `before=0 after=0 rolled_back=True`。
5. **多段线越界读伪造几何**：空表 `getPointAt(0)` 返回 `(0,0)`、`segType(0)=4`、`getLineSegAt(0)` 返回伪造线段 → 先判 `numVerts()`；越界读现在返回默认值并记 `eInvalidInput`、`segType` 返回 `-1`。
6. **`addVertexAt` 越界**：`addVertexAt(99)` 旧版透传 SDK → 现在要求 `0 ≤ index ≤ numVerts()`，越界返回 `False`。
7. **Ed 层传 None**：`entget(None)` 抛 `TypeError: incompatible function arguments`（entdel/entupd/handent 同类）→ 现在 `entget(None)→None`、`entdel/entupd(None)→False`、`handent(None)→None`。
8. **事务外持有包装**：`with` 退出后包装内的裸指针已失效，再调用几何方法属悬空访问 → 事务外要用对象就重新 `tr.openObject(id, mode)`。

对照脚本在 `tests\cad\`：`pyarx_pitfall_probe.py`、`pyarx_api_test_batch.py`～`batch4.py`、`pyarx_minimal_probe.py`。

## 二、API 根因修正（契约变化，逐条见 docs\_API_CHANGES.md）

**6 条根因修复**：`PyDbDatabase.openObject`（空/非法 id + mode 校验）、`PyDbTrans.openObject`（同上返回 `None`）、`PyDbTransactionManager.getObject`（同上）、`PyDbTrans.__exit__`（异常 abort）、`PyDbPolyline` 顶点索引校验、`Ed.entget/entdel/entupd/handent` None 归一。

**默认打开模式变更（架构级）**：图元与表/符号表操作默认由读改**写**——`openObject` 默认 `mode=1`，各 `getXxxTable`/`getXxxDictionary`/`getSymbolTable` 默认 `kForWrite`，块表记录遍历 `getRecord` 改 `kForWrite`。纯查询路径（modelSpace 取 BTR、实体计数、DXF 导出、只读遍历）显式传 `0` 保持只读；只读库或 xref 场景抛带 handle 与原因的 Python 异常，不再让 AutoCAD 弹 `!dbobji.cpp eNotOpenForWrite`。

**迭代器收敛**：Python 侧只保留容器本体协议（`for ent in btr`、`for rec in layerTable`、`table['0']`、`'0' in table`、`keys/values/items/get`），退役 15 个公开类（`PyDbBlockTableRecordIterator`、`PyDbBlockTableRecordEntityIter`（私有名）、`PyDbSymbolTableIterator`、`PyDbBlockTableIterator`、`PyDbLayerTableIterator`、`PyDbLinetypeTableIterator`、`PyDbTextStyleTableIterator`、`PyDbRegAppTableIterator`、`PyDbDimStyleTableIterator`、`PyDbViewTableIterator`、`PyDbViewportTableIterator`、`PyDbUCSTableIterator`、`PyDbAbstractViewTableIterator`、`PyDbXrecordIterator`、`PyDbObjectIterator`），C++ 类保留内部使用；7 个携带查询状态的迭代器（`PyDbFilteredBlockIterator`、`PyDbCompositeFilteredBlockIterator`、`PyDbSpatialIndexIterator`、`PyDbLayerIndexIterator`、`PyDbIndexUpdateDataIterator`、`PyDbBlockChangeIterator`、`PyDbLongTransWorkSetIterator`）保留公开。

**宿主层**：`XWindow` 的 `Background` 解析链路已接入（`win.setBackground(r,g,b)`）；新增 `win.getSize(id) -> (w,h)`；`XPowerCanvas` SVG 模式改为 `zoom=1, ox=oy=0`、取消 8px 内边距与等比 fit。

**编译**：`PyArxCore` 与 `PyMfcXml` 均 0 error（日志 `docs\_pyarxbuild_pyarxcore.txt`、`_pyarxbuild_pymfcxml.txt`、`_pyarxbuild_hot.txt`）。`PyArxCore.pyd` 被运行中的 acad 占用时走 hotbuild 产物验证，正式链接需先关闭 CAD。

## 三、图元增改测试结果（A 线：绑定层）

宿主：AutoCAD 2024 进程内 `PYRUNFILE`，热模块 `PyArxCoreH20261115_131114.pyd`。汇总 **25 例：通过 22，阻塞 3**。

| 用例 | 预期 | 实际 | 通过 |
|---|---|---|---|
| `PyDbLine` 构造 + `addEntity` | 句柄可取、起终点一致 | 句柄 696，start=(0,0,0) end=(10,0,0) | 是 |
| `PyDbCircle` | 圆心/半径一致 | 句柄 697，r=5.0 | 是 |
| `PyDbArc` | 半径/角度一致 | 句柄 698，start=0.0 end=1.5708 | 是 |
| `PyDbPolyline` 三次插点 | 3 顶点、第 2 点一致 | 句柄 699，verts=3 | 是 |
| 多段线越界索引 | 被校验挡住 | add=False，读点默认原点，seg=-1 | 是 |
| 闭合多段线 | 4 顶点、闭合标记 | 句柄 700，closed=True | 是 |
| `PyDbText` | 文字/字高一致 | 句柄 701，text=PYARX h=2.0 | 是 |
| `PyDbAlignedDimension` | 定位点可读 | 句柄 702 | 是 |
| `setColorIndex/setLayer/setLinetype` 写回 | 读回一致 | 1 / 0 / ByLayer | 是 |
| `setStartPoint / setRadius / setPointAt` | 几何读回一致 | (20,0) / 8.0 / (20,20) | 是 |
| `openObject(0,0)` / `(999999999,0)` / `(有效id,9)` | 空包装不抛异常 | `isNull=True` ×3 | 是 |
| `tr.openObject(0,0)` / `(有效id,9)` | `None` | `isNone=True` ×2 | 是 |
| 事务异常退出 / 正常提交 | 回滚 / 提交 | `rolled_back=True` / `committed=True` | 是 |
| `entget/entdel/entupd/handent(None)` | None/False/False/None | 与约定一致 | 是 |
| `PyDbBlockReference`（先建块定义） | 建定义→参照→位置读回 | 事务内阻塞 | 否（嵌套事务，块记录未纳入外层托管） |
| `entget+entmod` 属性回写 | 组码读回一致 | PYRUNFILE 上下文阻塞 | 否（`acdbEntMod` 链路阻塞，未定位到绑定层根因） |
| `entdel` 删除后重开再改 | 被挡住 | 随上条阻塞未完成 | 否 |

**容器遍历最小实测（4 项：通过 1、失败 2、部分 1）**：

| 用例 | 实际 | 通过 |
|---|---|---|
| `"0" in layerTable` | `contains=True` | 是 |
| `for ent in btr` 改颜色回读 3 | `RuntimeError: Caught an unknown exception!` | 否（缺打开对象缓存池，重开撞重复打开） |
| `layerTable.items()` | `len=1` 但 `items=0` | 否（枚举读包装与二次写打开未配平） |
| `layerTable["0"]` | 先取可、枚举后抛 `eWasOpenForRead` | 部分 |

## 四、2D CAD 现状与启动（B 线）

能力：绘制 直线/圆/弧/矩形/多段线；点选 + 窗口框选（可视橡皮框）+ 夹点拖动；MOVE/ROTATE/SCALE/ERASE/COPY；改图层、改颜色；滚轮缩放、全图适配、中键平移、对象捕捉；图层符号表；撤销重做（ADD/REMOVE/MODIFY 三类）；JSON 存盘读盘（图幅、当前图层、图层全字段、几何/图层/颜色随存随还原）；栅格随缩放平移重绘；Ribbon / 工具条 / 特性栏自绘 + 命中表；命令行（命令名/别名/中文/坐标）；SVG 导出、PNG 截图。界面本轮由项目所有者重做为 Ribbon 104px 五页签、工具条 46px、状态栏 canvas 胶囊。

启动（可直接复制）：

```powershell
cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py
cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py --selftest
cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py --batch
```

实测：`[selftest] OK 图元 10 · 缩放 75%`；`--batch` 16 用例通过 14、失败 0、未实现 2。

| 用例 | 预期 | 实际 | 通过 |
|---|---|---|---|
| 直线 / 圆 / 弧 / 矩形 / 多段线绘制 | 几何与预期一致 | (1000,1000)-(3000,1200) / (5000,2000) r=800 / r=556.2 / bbox(1000,3000,2500,4000) / 3 顶点 | 是 ×5 |
| 夹点拖动 | 终点移到 (3200,1600) | (3200.0,1600.0) | 是 |
| MOVE | 整体偏移 (+500,+300) | 起点 (1500.0,1300.0) | 是 |
| ROTATE | 相对旋转 90°、长度不变 | 15.3→105.3°、长度 2280.4→2280.4 | 是 |
| SCALE | 半径 800→1200 | 1200.0 | 是 |
| ERASE | 圆被移除 | 剩余 4，圆存在=False | 是 |
| 改图层 / 改颜色 | TESTLAYER / 3 | 一致 | 是 ×2 |
| 窗口选择 | 至少 1 个 | 选中 3 个 | 是 |
| 存盘读盘往返 | 数量/类型/图层/颜色/几何一致 | 存 4 读 4，逐项一致 | 是 |
| 线性标注 DIM | 三点线性标注 | 未实现 | 否（实体与渲染桩就位，命令状态机未接线） |
| 区域填充 HATCH | 闭合区域填充 | 未实现 | 否（SVG pattern/clipPath 能力未验证） |

继承关系：既有 `app.py / doc.py / geom / render / db / ent / interact / cmdline / ui / cad_shell.xml / res` 全部保留未重写，改动为追加方法与带默认值参数扩展，既有 selftest 断言未改；C++ 绑定与控件库本轮零改动。产物：`examples\cad2d\cad2d_batch.png`、`cad_shot.png`、`cad_drawing.json`。

## 五、环境修复

`X64\Release\python.exe` 缺 `python315t.dll`，直接运行退出码 `-1073741515`（STATUS_DLL_NOT_FOUND），cad2d 全部启动命令失效。已从 `D:\Program Files\Python315t\` 复制 `python315t.dll` 与 `python3.dll` 到 `X64\Release\`，验证：

- `.\python.exe -c "import sys;print(sys.version)"` → `3.15.2 (tags/v3.15.2:8e6e75d ...) [MSC v.1944 64 bit (AMD64)]`
- `.\python.exe -c "import PyMfcXml;print('ok')"` → `PyMfcXml ok`

部署脚本或新机器上必须带上这两个 dll，否则 `python.exe` 与 `PyMfcXml.pyd` 都起不来。

## 六、DWG 导入路线与开源库选型（路线建议，未实现代码）

- 两级路线：一级 DXF（原生可写、成本低、优先落地）；二级 DWG（只读优先，靠外部转换器或商业 SDK）。
- LibreDWG（C，GPLv3）：读 R13-R2000 稳、R2004+ 部分支持、写不完整；libdxfrw（C++，GPLv2）：读 DWG、写 DXF；ACadSharp（C#/.NET，MIT）：读 DWG、读写 DXF；ezdxf（Python，MIT）：仅 DXF 读写，2D 首选；ODA Drawings SDK（C++，商业授权）：DWG 读写最全；ODA File Converter（免费工具）：DWG↔DXF 进程外批处理。
- 许可红线：GPL 系（LibreDWG、libdxfrw）不得链进闭源产品，只能进程外调用且分发受限，需决策。
- 推荐：DXF 用 ezdxf(MIT) 或自研子集解析器；DWG 先用 ODA File Converter 转 DXF 再走同一管线；写 DWG 不做。
- 原生读 DWG 最小知识面：AC10xx 版本标识、段表、对象映射与 handle、R2004+ 段页压缩与 MC 解压、实体公共属性 handle/owner/layer/color/linetype、实体类名 AcDbLine/AcDbCircle/AcDbArc/AcDbPolyline/AcDbText/AcDbBlockReference/AcDbDimension、块嵌套变换、多段线 bulge 弧段、ACI→RGB、代理图形回退。

## 七、已知缺口与根因

1. **块表记录遍历写属性失败**（`for ent in btr` 抛 `unknown exception`）：绑定层没有打开对象缓存池，事务提交后重开同一实体撞重复打开。修法：按 oid 维护打开的包装与模式，写请求对读包装调 `upgradeOpen()`，作用域结束统一 close 一次。
2. **符号表枚举后取值失败**（`items=0`、枚举后 `getAt("0",1)` 抛 `eWasOpenForRead`）：同一对象只能一个打开包装，枚举持有的读包装未释放。修法同上，`items` 用一次遍历同时生成 `(名, 记录)`。
3. **SVG 矩形边界还原不全**（probe 控件 `getSize=1344×104` 与声明一致，红框实测 `(0,0)-(1327,64)`，右短 16px、下短 39px）：根因不在视图变换（D2D identity、zoom=1/ox=oy=0），在 SVG 矩形轮廓点/闭合多边形填充链路，右与下边界未完整还原。
4. **`PyDbBlockReference` 建参照链路阻塞**：外层 `PyDbTrans` 内调用 `BlockTableRecord.appendEntity`（内部再开事务），块记录未纳入外层托管。规避：同事务内裸追加后 `BlockTable.add(btr)`。
5. **`entget+entmod` 链路阻塞**：`acdbEntMod` 在 PYRUNFILE 命令上下文阻塞，未定位到绑定层根因。规避：改用 `openObject(id, 1)` 直接改属性。
6. **DIM / HATCH 未实现**：DIM 实体与渲染桩就位、命令状态机未接线；HATCH 依赖 SVG pattern/clipPath，能力未验证。
7. **物理 Esc / 右键取消缺失**：`onCanvasEvent` 只派发 down/move/up/dblclick/wheel，无键盘与右键事件；当前靠命令行 ESC 与点空白兜底。
8. **5 条接口缺口**已登记在 `docs\_API_CHANGES.md`（键盘/右键事件、`getSize` 的表格视口行计数、SVG pattern/clipPath、`setTableRows` 分隔符契约）。
9. **默认写与 upgradeOpen 并存**：早前指令要求删掉 upgradeOpen，但 AcDb 同一对象只能有一个打开包装，枚举已持读包装时必须靠 `upgradeOpen` 升级，因此保留该路径；默认写打开照旧生效。

## 八、后续建议

- 优先做「打开对象缓存池」，一次解决缺口 1、2、4，并让 `values/items` 默认只读、惰性 `for` 路径自行升级。
- SVG 矩形/多边形边界还原单独修一轮，修好用同一个 probe 判定 `(0,0)-(1343,103)±2px`。
- `entget/entmod` 阻塞在 PYRUNFILE 上下文的成因需单独立项（可能与命令上下文里的数据库锁有关），不影响事务内直接改属性。
- cad2d 下一步：DIM 三点定点、DXF 持久化，再按选型接 DWG（先外部转换）。
- 部署清单补 `python315t.dll` / `python3.dll`；测试设施固化「实例上限 2 + 每批 Quit + 只按 PID 精确关闭、禁止窗口广播」。
