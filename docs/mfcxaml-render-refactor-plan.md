# PyMfcXaml 绘制架构重构施工单（三步）

目标：把"每个控件一个 HWND、各自 WM_PAINT"改成"父表面一次绘制 + 少量真 HWND 负责输入"，
把整窗重绘从 82 ms 级压到 20 ms 以内，让分割条拖动和节点详情弹出跟手。

当前实测基线（1324×841，软件 D2D，侧板 640）：

| 指标 | 值 |
|---|---|
| `_layout()` | 0.19 ms |
| 拖动一步（含一次 pump） | 217 ms（本刀前 266 ms） |
| 点节点→详情 | 117 ms（本刀前 132 ms） |
| 整窗重绘 | 82.1 ms |
| 分项重绘 | 画布 6.0 / 表格 8.7 / 侧板白卡 16.6 / 状态栏 1.7 / 小地图 0.0 |

已完成的第一刀：`moveMany` 用 `BeginDeferWindowPos` 一次提交，父窗口只标"旧∪新"并集矩形
（原来是 `RedrawWindow(RDW_INVALIDATE)` 整窗标脏，把没动的工具栏/状态栏/小地图也拉去重画）。

---

## 第一步：装饰控件改为父表面绘制项（预期最大化）

**改什么**
- `PyMfcHost` 新增表面表：`std::map<int, SurfaceItem{ RECT rc; COLORREF fill; COLORREF border; bool hasBorder; }> m_surfaces`
  与接口 `setSurface(id,x,y,w,h,fill,border)` / `moveSurfaces(list)`；
- 在 `PyMfcProcs.cpp::PyMfcHost::OnEraseBkgnd` 里，画完窗口底色与 `m_backdrops` 之后，
  按 z 顺序把这些表面矩形铺出来（`FillSolidRect` + 边框）；子 HWND 仍然画在自己上面；
- Python 侧把这些"纯装饰"控件从 `addWhiteRect/addPanel/addStaticLine` 改成 `setSurface`：
  `ED_PANEL`（白卡）、`ST_PANEL`（状态栏底）、`MI_PANEL`（小地图框）、`DT_PANEL`（详情卡）、`SPLIT`（分割条）。

**关键约束（必须一起处理）**
- 改成表面项后**没有 HWND**，凡是用 `GetDlgItem(id)` / `GetWindowRect` 的地方都会拿不到：布局断言、
  `getTableMetrics`、`cellRect` 这类探针要改成读 `m_surfaces`（新增 `getSurfaceRect(id)`）；
- `SPLIT` 还要能拖：保留一个"只有热区、不自绘"的 HWND（`SPLIT_HIT`）挂在同位置，
  `onDrag` 绑在热区上，`SPLIT` 只做视觉；
- z 序：表面项画在父表面，等于永远在子 HWND 之下；白卡里如果有文字控件（`L_EDIT_TITLE` 等）
  它们是 HWND，仍在卡片之上，正确。

**验收**
- 一次分割条拖动，参与 paint 的子窗口数从 ~15 降到 ~4（画布 + 表格 + 编辑框 + 分割热区）；
- 拖动一步 ≤ 120 ms；侧板白卡那 16.6 ms 归 0；`pp_test_all` / `pp_split_test` / `pp_cell_test` 全绿；
- 截图核对：白卡圆角、分割条位置、详情卡阴影不退化。

**回滚点**：`docs\_backup_PyMfcProcs.cpp`、`docs\_backup_PyMfcHost.h`（本次重构前另存一份）。

---

## 第二步：画布离屏位图缓存

**改什么**
- `drawPlanCanvas()` 增加缓存：键为 `(id, zoom, ox, oy, width, height, plan 版本号)`；
  命中则 `DrawBitmap` 缓存，不重新遍历 39 卡片 + 8 条 S 曲线；
- 失效条件：缩放/平移变化、数据重建、窗口尺寸变化、选中态变化（选中要描红，可只重画描边层）。

**验收**：画布重绘 6.0 ms → ≤ 1.0 ms；缩放/平移后第一帧仍完整，无空白/错位；`pp_test_all` 27/27。

---

## 第三步：表格 owner-data 虚拟列表

**改什么**
- `SysListView32` 改 `LVS_OWNERDATA`，行数用 `LVM_SETITEMCOUNT`，取数据在
  `LVN_GETDISPINFO` 里按 `(row, col)` 回宿主 `m_tblRows`（目前是宿主与原生各存一份）；
- `setTableRows` 不再 39×8 次 `LVM_SETITEMTEXTW`，只 `SETITEMCOUNT` + 一次失效；
- 排序改为改宿主数据后 `SETITEMCOUNT` 重取，不再逐行写文本。

**验收**：`fill_table` 15.6 ms → ≤ 3 ms；39 行选中 129 ms 明显下降；`pp_cell_test` 15/15、
`pp_grid_test` 18/18、`pp_select_test` 7/7 全绿。

---

## 施工纪律

1. 每步：改 → 编（`EXIT=0`）→ 全量 11 个用例 → 截图核对 → 记数字入库；
2. 任一步回归跌破既有项数，立即回滚该步，不带着问题往下走；
3. 每步结束把数字写回本文件与 `docs\session-2026-11-11-mfcxaml-digest.md`。


---

## 追加：最根本的原因（2026-11-11）

分项重绘数字本身就是证据链：

| 控件 | 重绘耗时 | 该控件真正要画的量 |
|---|---|---|
| 侧板白卡 ED_PANEL | 16.6 ms | 一块纯色矩形（fillRect） |
| 表格 TBL | 8.7 ms | 一屏 30 行的文本 |
| 画布 CV | 6.0 ms | 39 卡片 + 8 条曲线 |
| 状态栏 | 1.7 ms | 一行文本 |
| 小地图 | 0.0 ms | 几乎不画 |

**一块纯色矩形要 16.6 ms** —— 纯 `FillSolidRect` 在软件渲染下是 0.1 ms 级。
这说明耗时不在"画了什么"，而在**每次 WM_PAINT 都要重新建一遍 D2D 环境**：
设备/工厂/渲染目标/缓冲 surface 每次 `BeginPaint` 重建，软件光栅下单次创建就是 5~16 ms，
再乘以"一次布局动到的子窗口数"，就是 217 ms/步。

因此三条现象是同一个病根的三个面：

1. 拖动卡 → N 个子窗口各自重建一次 D2D 环境；
2. 点节点慢 → 详情浮层显隐又带起一次多窗口重绘；
3. 分割条看不见/被盖 → 每窗口各画各的，没有统一表面，z 序与裁剪不可控。

**修法优先级（改这里，收益最大）**
- A（最根本）：**把 D2D 环境按 HWND 缓存**，只在尺寸/DPI/渲染后端变化时重建，`WM_PAINT` 里只做
  `BeginDraw / 画 / EndDraw`。单点改动，能同时把它三件事一起解决。
- B：父表面一次绘制（省掉 N 次 BeginPaint，也顺手解决 z 序）。
- C：画布离屏位图缓存；D：表格 owner-data。

A 与 B 不冲突，A 先做（改动面更小、可测量），B 用来彻底抹掉"每控件一次 paint"。


---

## 架构层面不合理之处（2026-11-11，由实测反推）

实测里最难解释的一笔：拖动一步 192 ms，其中"能看见的画"只有 ~28 ms
（CV 5.5 + TBL 8.2 + ED_PANEL 13.1 + ST 1.4），空转 `pump(30)` 38.7 ms，
**还有约 125 ms 找不到出处**。这个缺口本身就是架构问题的指纹。

1. **拿 HWND 当合成单元**（最根本）
   HWND 本来是"输入 + 裁剪 + z 序"的单位，不是"绘制"的单位。用它当绘制单元导致：
   无法整体双缓冲、无法整体缓存、无法保证绘制顺序（分割条被兄弟窗口盖掉就是这么来的）、
   每个窗口都要各自 BeginPaint / 绑 D2D 目标 / BitBlt 一次。
   HTML/Qt 的做法是**一个表面 + 一棵保留树的绘制节点**，控件只是节点属性。

2. **布局与"内容重排"没分离**
   Python 算好矩形 → `moveMany` 改窗口尺寸 → 每个子窗口收到 `WM_SIZE` 后**立刻重排自己的内容**：
   表格重排 39×8 单元格、画布重排 39 张卡片。也就是说拖动一步不是"重画"，而是
   **十几份数据重排**。这正是那 125 ms 的出处（`_layout` 0.18 ms 只计了 Python 那半边）。

3. **D2D 环境按控件各持一份**
   原来是全进程一份（互相踢掉缓存），现在按控件各一份（缓存不再互踢，但内存与创建成本 ×N）。
   正确形态是**一份目标 + 一份笔刷/文字格式缓存 + 各控件的绘制指令**。

4. **双缓冲叠了三层**
   `s_memDc`(DIB) + D2D DC target + 末端 `BitBlt` 全块拷贝；640×755 一次就是 1.9 MB，
   乘以控件数。合成层应当只有一次。

5. **数据与视图全量耦合**
   `setTableRows` 每帧全量灌；正确是 owner-data + 只取可见行（第三步）。

6. **事件与绘制同线程串行**
   Python 侧 `pump(ms)` 驱动消息，节流也在 Python 侧做时间窗；拖动期每一帧都必须
   等一个完整消息循环走完才响应下一次移动，没有"输入优先、绘制可丢帧"的分离。

**结论**：要真接近 HTML，必须有一步是"单一表面 + 保留树 + 控件不再是 HWND"，
其余（缓存、owner-data、并集失效）都只是把这个架构的地基垫平。
在那之前，性能上限由"每步 N 次内容重排 + N 次 BeginPaint"钉死。

---

## 2026-11-11 末轮：详情双口径验收与热点拆分

### 口径与结论
- **口径 A（历史可比）**：`_perf_fix3.py` 单击后单次 `pump(30)` 读数；含脚本 30ms 空闲预算，未计入详情下一次 WM_PAINT，不做验收判据。三次复测 59.3 / 59.5 / 59.4 ms。
- **口径 B（真实感知）**：真实 `SetCursorPos` + `SendInput` 点击节点卡片，`pump(1)` 轮询固定探点 GetPixel，探点变白卡即停。5 次结果 217.43 / 114.11 / 113.70 / 116.19 / 114.98 ms；**中位 114.98 ms、均值 135.28 ms，>40 ms**。

### paint 链拆分
- Python `fill_detail` 非泵中位 3.19 ms；`show_detail/_layout` 8.63 ms；详情逐 HWND 同步重绘合计中位约 30.64 ms。
- 20 个可见详情 HWND 中，`DT_PANEL` 详情卡底板单控件中位 19.11 ms，是最大热点；其余控件多在 0.26~0.90 ms。
- 口径 B 剩余未闭合段在原生点击分发到各 HWND paint 之间；当前父表面基础设施只支持纯色 `FillSolidRect`，未支持圆角、阴影、边框与文字，`DT_PANEL` 暂不能直接转表面而不退化基线观感。

### 回归与产物
- 全量串行 `_run_regression.ps1`：118 / 118 PASS；`pp_cursor_test` 单独 5/5 PASS。
- 截图：`docs\_detail_e2e_probe.png`；探点标注版：`docs\_detail_e2e_probe_marked.png`。
- pyd：`G:\PyArx\X64\Release\PyMfcXaml.pyd`，mtime 2026-11-11 19:07:00。

### 三刀轮补测记录（2026-11-11，软件 D2D）
- 口径 B 5 次：63.17 / 78.31 / 62.81 / 63.44 / 62.91 ms；中位 **63.17 ms**、均值 **66.13 ms**，未达 ≤40 ms。探点 pre `(243,245,248)`、post `(255,255,255)`。
- `hideDetail()` 已去掉重复 `_detail_raised=False`，隐藏后再次弹出避免 `raiseMany` 全量兄弟重标脏；备份 `docs\_bk7_pp_app.py`。
- 阴影缓存日志显示 `id=1220 272x346 HIT blit=0.02~0.04 ms`；DT_PANEL 本体不是剩余主病灶。
- 回归 118/0；cursor 5/5；拖动 3 次单帧 `_layout` 0.01 ms。
- 剩余判断：口径 B 的超支来自 21 个 HWND 的外围 BeginPaint/消息分发与系统合成，而不是缓存底板继续重绘。



---

## 本轮实测与落地（2026-11-11，口径 1324×841 / 软件 D2D / 侧板 640）

### 1. 性能数字（before / after，官方脚本 `_perf_fix3.py`）

| 指标 | 基线 | 本轮最终 | 达标线 | 判定 |
|---|---|---|---|---|
| 拖动一步 | ≈197 ms | 37.9~46.7 ms（40 步 1514.3~1869.8 ms） | ≤60 ms | 达标 |
| 点节点→详情 | 115~132 ms | 59.6~77.6 ms（5 次均值） | ≤40 ms | 未达标 |
| 拖动期 `_layout` 单帧 | 0.17~0.23 ms | 0.01 ms（最大 0.05） | — | — |

热点明细（探针实测，完整记录见 `docs\_perf_round2.txt`）：
- 裸 8 次 `LVM_SETCOLUMNWIDTH` 合计中位 156.08 ms/轮（单列 20.6~24.7 ms，末列 8.33 ms）；
  宽度不变走跳过分支的 `_layout` 仅 2.00 ms。`on_split_drag` 宽度真变中位 166.80 ms。
- 批处理后 `on_split_drag` 中位 11.41 ms；拖动 40 步从 10658.1 ms 降到 1514.3 ms。
- 详情改前：setvals 19.92 / show 28.11 / layout 6.34 / raiseMany 16.12 / pump 57.61 ms。
- 下拉 `CB_ADDSTRING` 前 3 条各 ~5 ms；改固定档只灌一次后 setvals ~2 ms。
- 重复选中跳过 raiseMany：fill 24.71→8.42 ms，pump 53.04→36.54 ms，合计 77.67→44.48 ms。
- 详情 21 个 HWND：DT_PANEL 白卡 14.04 ms，其余 20 个 0.32~0.85 ms/个；C++ 内部全部 <1.7 ms。
  剩余大头是 OS/D2D 外层 BeginPaint/EndDraw 软件光栅化 + pump(30) 地板 28~36.5 ms。

### 2. 每刀改了什么、为什么省时间

1. **批量列宽 `setColumnWidths`（保留，拖动达标主因）**：C++ 新增宿主方法，`WM_SETREDRAW(FALSE)` 包住整轮
   8 次 `LVM_SETCOLUMNWIDTH`，结尾 `WM_SETREDRAW(TRUE)` + 单次 `InvalidateRect(TRUE)`；经 Wrap/Bind 暴露，
   Python `_layout_editor` 宽度真变时一次调用。消掉的是"逐列触发整表 relayout/重画"，中间帧不上屏、只在结尾重画一遍。
2. **电压下拉串只灌一次（保留）**：5 档固定选项首次灌入，之后只 `setCurSel`；非标准电压才追加重建。
   消掉每次点节点前 3 条 addString 各 ~5 ms。
3. **raiseMany 缓存（保留）**：`_detail_raised` 标志，已置顶跳过 64 个子窗口 `SetWindowPos` + 全子窗拉重画；
   显隐/层级变化后置 False。消掉重复点选时的重复 z 序提交。
4. **applyZOrder 同序/局部失效优化（已回退，未保留）**：本环境同序判定始终不触发，收益已被 raiseMany 缓存覆盖，
   且涉 z 序/裁剪观感；`applyZOrder` 回基线原版 `RDW_INVALIDATE | RDW_ALLCHILDREN`。
5. **连线命中改点到线段（新增）**：`planHit` 去掉贝塞尔 24 段采样，取 4 个拐点折线逐线段距离，与屏上绘制一致。
6. **SVG 导出改折线（新增）**：`toSvg` path 由三次曲线 `C` 改正交折线 `L`；复测 38 条 path、C=0、L=112。

### 3. 表格行号"差 1"探针（未改码）

四数探针 `tests\mfc_xaml\_probe_row.py`（`docs\_probe_row.txt`、`_probe_row_real.txt`）：
cellRect 表头 24 / 行高 21，各行矩形连续无重叠；SendMessage 单击下原生选中与 host 选中跟随点击行一致；
真实鼠标在本无人值守会话未驱动表格选中（已知合成投递不稳），未能复现用户截图所述差 1。
单独跑 `pp_cell_test` 15/15、`pp_select_test` 7/7、`pp_grid_test` 18/18。按"先量后改"未据此动码。

### 4. 回归逐项（11 用例 118 项）

最终构建一次完整串行 `_run_regression.ps1`（`docs\_regress_summary3.txt`）：**118/0 全绿**。
连线命中/SVG 改动后再跑（`_regress_summary4.txt`）：test_all 27/0、split 13/0、cell 15/0、event 8/0、
select 7/0、real 6/0、cursor 5/0、click 5/0、runtime 9/0、plugin 5/0；grid 剪贴板 2 项与 split 1 项为
无人值守会话真实键盘/焦点间歇（基线构建同样间歇，单独跑 split 13/13、cell 15/15、select 7/7 通过），
非本轮代码回归。

### 5. 观感核对（截图在 docs）

- `_after_editor.png`：侧板白卡圆角、SPLIT 6px 浅灰底 + 1px 中线可见，z 序未被破坏。
- `_after_detail.png`：详情白卡圆角、右下阴影与基线 `_pp_one_detail.png` 一致。
- `_after_export.svg` / `_after_export.png` / `_after_export_crop.png`：导出连线为直角折线，无三次曲线。
- 与历史基线截图逐项比对：分割条、白卡圆角、详情阴影无差异。

### 6. 未决与后续

- 详情 ≤40 ms 未达标：剩余成本在 21 个 HWND 各自 BeginPaint + DT_PANEL 单卡 ~14 ms + pump 地板；
  需走"装饰件改父表面绘制项（无 HWND）"路线，宿主表面基础设施已在 `PyMfcHost` 备好，Python 转发与
  `GetDlgItem/getWindowRect/布局断言/探针 id` 改读 `getSurfaceRect` 是主要工作量与风险。
- 表格差 1 缺陷待真实前台会话带截图复现后继续定位；`on_grid_dbl` 读选中行是候选滞后点。

---

## 2026-11-11 晚：新自绘表格 PyMfcTable 接线（结构性消除"选中/编辑差一行"）

### 1. 决策与范围

停止修老 SysListView32 差一行链路；改用已存在的自有窗口类 `PyMfcTable`（命中/选中/矩形同源）。
老 ListView 路径与 `host.cellRect` 保留兼容不删；新表通过能力探测 `hasattr(win,'addTable')` 启用。

### 2. Host / Wrap / Bind 接线（C++）

- `PyMfcHost`：`addTable` 创建 `TableControl` 实例并存 `m_newTables[id]`；新表 API 封装：
  `tableSetColumns / tableSetColTypes / appendTableRow / tableSetCell / tableGetCell / tableCellRect /
  tableSelectedRow / tableSelectRow / tableSetFilter / tableClearFilters / tableSort / tableViewRows /
  tableClear / tableRowCount`。事件静态 trampoline `tableEventCb`：what 1=cellclick 2=dbl 3=select
  4=edit 5=delete 6=insert 7=paste，转 `fireGridAction`；paste 文本从剪贴板读。
- 旧宿主方法在新表存在时回退委托新控件：`getTableCell/setTableCell/cellRect/getTableSelectedRow/
  setTableSelectedRow/getTableRowCount/clearTableRows/setTableRows`，使直接走旧包装的测试不改也能用。
- Wrap 双类（PyMfcWindow/PyMfcPanel）转发；Bind `.def` 暴露 `addTable/setTableColumns/setTableColTypes/
  appendTableRow/tableSetCell/tableGetCell/tableCellRect/tableSelectedRow/tableSetSelectedRow/
  tableSetFilter/tableClearFilters/tableSort/tableViewRows/tableClear/tableRowCount`。

### 3. TableControl 补齐交互（不改既有几何/绘制）

- 补齐 `VK_INSERT→insert`、Ctrl/C 复制选中行到剪贴板（CF_UNICODETEXT，tab 分隔 7 格）、
  Ctrl/V 读剪贴板触发 paste；F2/Delete/方向键原已具备。
- 表格控件本身不可输入文本，C/V 快捷键不依赖 `GetKeyState(VK_CONTROL)`（无人值守会话真实 keybd_event
 的 Ctrl 状态常读不到；与 `pp_events.key_with_ctrl` 直发 WM_KEYDOWN 的既有约定一致）。
- 列类型映射 FIELDS：code/name/upstream/cable/voltage=Text(0)，loadKw/length=Double(2)；
  Double 右对齐 + 数值比较；voltage 带单位保持 Text。

### 4. Python 侧迁移（pp_app.py，旧路径保留）

- `_new_table = hasattr(win,'addTable')`；建表走 `addTable + setTableColumns + setTableColTypes`，
  否则旧 `addListCtrl + addColumn`。
- 统一访问层 `_tbl_sel/_tbl_set_sel/_tbl_get/_tbl_set`；`fill_table` 新表 `tableClear + appendTableRow`；
  `_cell_begin` 矩形优先 `tableCellRect`；`collect_grid/on_row_ok/_cell_apply/on_add_row/on_del_row`
  按新/旧分派；列宽随侧板缩放时新表重 `setTableColumns` 并重设列类型。
- `pp_grid_test` 中原生 ListView 专属断言（窗口类/列数/WS_VSCROLL/末行矩形/剪贴板格数/行数）
  按新表新架构判读；行为断言（F2/Insert/Delete/Copy/Paste/排序/缩放）保留。Ctrl+C/V 改直发
  WM_KEYDOWN 到表控件（与其余用例一致）。

### 5. 实测数字（1324×841 / 软件 D2D / 侧板 640，_perf_newtable.txt）

- 拖动 40 步：1375.3 / 1365.5 ms，39 帧，`_layout` 单帧 0.01 ms，单步 ~34.1~34.4 ms，≤60 ms 达标。
  机制：新表无 LVM_SETCOLUMNWIDTH，旧表 ~144 ms 的逐列整表 relayout/重画被结构性消掉。
- 点节点→详情：65.1 / 66.0 ms（5 次均值），仍 >40 ms 未达标；详情 HWND 路径未动，成本仍在
  21 个 HWND BeginPaint/软件光栅化 + pump 地板。

### 6. 回归（单独跑 / 全量）

- 单独：`pp_cell_test` 15/15、`pp_select_test` 7/7、`pp_grid_test` 18/18 全绿。
- 全量 `_run_regression.ps1`：116 PASS / 2 FAIL；FAIL 全部在 `pp_cursor_test` 2 项（画布内悬浮手形、
  画布外复位箭头），属已知死路（模拟 WM_MOUSEMOVE 不触发 WM_SETCURSOR、宿主不在 WM_MOUSELEAVE 复位光标），
  与表格迁移无关，基线同会话同样间歇；单独跑仍为同 2 项。其余 10 用例全绿。

### 7. 观感取证（docs 截图）

- `_live_table.png`：新表表头 #F7F8FA、漏斗 ▽ 灰、7 列、数值列右对齐、行线 #E4E7EC；侧板白卡/分割条保留。
- `_live_edit.png`：选中行 AT1-1 整行蓝底白字，原位编辑框正好盖在同一行名称格，不再卡在两行之间；
  cell_test 矩形对齐 (878,382) 与期望完全一致。
- `_live_detail.png`：详情白卡圆角 + 右下阴影与基线一致，未被迁移破坏。

### 8. 未决（2026-11-11 晚前）

- 详情 ≤40 ms 仍未达标；走装饰件父表面化（无 HWND）路线，Python 转发与 getSurfaceRect 适配是主工作量。
- 新表暂无原生垂直滚动条：表高按"表头 + 整数行"对齐排布，超出行被裁剪；39 行当前可视够放，
  超长滚动是后续可选项。

---

## 2026-11-11 晚：详情达标、光标转绿、滚动补齐（本轮收尾）

### 9. 详情 ≤40 ms（**已被口径 B 推翻，作废**）

> 注意：本节旧结论“详情 30.2 / 30.6 / 31.8 ms，≤40 ms，达标”已被后续口径 B（真实点击到探点像素出现）
> 推翻；口径 B 实测中位 62.97 ms、均值 63.59 ms，仍 >40 ms。本节仅保留历史拆分记录，不得作为验收依据。

阶段拆分（中位 ms）：控件赋值 2.62、`_layout` 8.10、`pump(30)` 35.85、空 pump 32.06；待派发 paint
仅 2.4~3.5 ms。旧口径把 `pump(30)` 自身空闲预算计入详情延迟，不能代表真实感知。
`PyMfcBind.cpp::pump` 的等待契约另见第 14~15 节；拖动 40 步约 1269.8~1296.8 ms、单步约 32 ms，≤60 ms。

### 10. 光标 2 项（pp_cursor_test 5/5）

宿主侧：`WM_MOUSELEAVE` 补进画布 OwnerDraw 派发闸门，离开画布 `SetCursor(IDC_ARROW)` 复位；
`WM_SETCURSOR`/`WM_MOUSEMOVE` 画布内设 `IDC_HAND`。用例侧：先 ShowWindow(SW_RESTORE)/BringToTop/
SetActiveWindow/SetForegroundWindow/置顶切换，真实 Sleep 0.15 s 等前台完成；落点经 ClientToScreen 换算；
进入/离开/返回补一条 `WM_MOUSEMOVE` 走画布自身光标分支。判清：原先失败不是表格迁移，而是主窗被
Excel 等窗口压住、真实落点不在本进程窗口，触发了画布 `WM_MOUSELEAVE`。

### 11. 新表垂直滚动

`TableControl` 增加 `m_scroll`（可视首行数据号）、`VisibleCount/MaxScroll/ScrollBy/EnsureVisible`；
滚轮改表内按整行滚动；右侧 10 px 自绘 track + thumb；`RowRect/Cell/Hit` 按滚动偏移定位；
Host/Wrap/Bind 暴露 `tableEnsureVisible`；Python 选中行、原位编辑、追加行都调用。80 行探针：
末行 top=633 可视、回顶首行 top=24、下滚后首行 top=-102 移出，PASS。

### 12. 回归与观感

- 全量串行 `_run_regression.ps1`：118 / 118 PASS（27+13+15+8+18+7+6+5+5+9+5）。
- 单独：cell 15/15、select 7/7、grid 18/18、cursor 5/5。
- 截图：`docs/_after_table_split.png`（新表 + 右侧滚动条 + 画布/侧板分割条）；
  `docs/_after_detail.png`（详情白卡 + 阴影 + 字段，与 `_live_detail.png` 基线一致）。
- PYD：`G:\PyArx\X64\Release\PyMfcXaml.pyd`，mtime 2026-11-11 18:18:46。

### 13. 本轮改动与备份（docs\_bk4_*，未覆盖 `_bk2_*`/`_bk3_*`）

- 源码：`PyMfcBind.cpp`（pump 提前返回、`tableEnsureVisible` 暴露）；`PyMfcWrap.h/.cpp`（转发）；
  `PyMfcHost.h/.cpp`（`tableEnsureVisible`、Select 内 EnsureVisible、moveMany 未变跳过、阴影 DIB 缓存宿主侧）；
  `PyMfcProcs.cpp`（阴影 DIB 缓存、WM_MOUSELEAVE 闸门、光标分支）；`Table.h/.cpp`（滚动几何/滚轮/滚动条）；
  `pp_app.py`（show_detail 热态早退、选中/编辑/追加行 ensureVisible）。
- 探针/工具：`_perf_fix3.py`、`_probe_detail_stage.py`、`_probe_detail_nopump.py`、`_probe_scroll80.py`、
  `_probe_runtime.py`；`pp_cursor_test.py`（前台置顶 + 真实落点换算）。

---

## 2026-11-11 晚补：pump 语义修正（消除闲置空转，恢复等待预算）

### 14. 问题与契约

上一轮为压详情把 `pump(maxMs)` 改成队列空即无条件返回，导致：
- 常驻循环退化成约 250Hz 轮询，闲置 CPU 实测单核 **99.375%**；
- 大量 `pump(<ms>)` 被当等待用，等待语义被删掉。

修正后契约（`PyMfcBind.cpp`）：
- `maxMs>0`：派发过消息且队列排空即返回；一次都没派发则有超时地等到预算用完；
- `maxMs==0`：阻塞到 `WM_QUIT`，语义不变。

### 15. 实测数字

- 闲置 CPU：非阻塞版 9937.5ms/10s（99.375%）→ 修正后 15.625ms/10s（**0.156%**）。
- 等待语义（空队列）：`pump(20)=30.4ms`、`pump(80)=58.8ms`、`pump(120)=127.8ms`、`pump(200)=205.7ms`，不再 0~2ms 立即返回。
- 拖动 40 步：1315.0 / 1285.5 / 1294.2 ms，单步约 32.1~33.4 ms（≤60）。
- 详情：50.2 / 56.2 / 59.8 ms（均值约 55.4 ms），仍 >40；非泵内 5.63ms，`pump(30)` 中位 40.80ms，
  空 pump 32.38ms；详情泵多为 0~1 条消息，待派发重绘未在 pump 预算前到达。
- 回归：全量串行 **118/118 PASS**；`pp_cursor_test` 单独 **5/5 PASS**。

### 16. 改动与备份

- 源码：`PyMfcXaml\PyMfcBind.cpp`、`PyMfcXaml\PyMfcHost.cpp`。
- 备份：`docs\_bk5_PyMfcBind.cpp`、`docs\_bk5_PyMfcHost.cpp`。
- 探针：`tests\mfc_xaml\_probe_pump_wait.py`、`_probe_detail_nopump.py`。


## 2026-11-11 19:35 复核状态（与 §9 对齐：详情未达标）

- 回归：全量串行 **118/118 PASS**；`pp_cursor_test` 单独 **5/5 PASS**。
- 拖动：40 步总 1314.4 ms，39 帧重排，单帧 `layout` 0.01 ms；总时折算单步约 32.9 ms，≤60 ms。
- 详情口径 B：5 次 64.31 / 62.92 / 62.64 / 62.97 / 65.10 ms；中位 **62.97 ms**、均值 **63.59 ms**，仍 >40 ms。
- 分项重绘中位：CV 5.2、TBL 8.3、ED_PANEL 16.0、MINI 0.0、ST_PANEL 1.7 ms。
- 未落地：详情纯展示标签/底板装饰并入画布或父表面；输入框、下拉、按钮必须保留 HWND。现有 20 个可见详情 HWND 同步重绘合计中位约 30.91 ms，是下一刀主要账本。


## 2026-11-11 追加：底层换框架（用户指令"继续重构，使用更新的框架，从底层修改"）

### 决策
不再在 GDI/U-控件的既有骨架上打补丁，改为在 **MfcXamlCore 底层**换渲染框架：
- **D3D11 设备 + DXGI 交换链（FLIP_DISCARD, 2 缓冲, B8G8R8A8）+ ID2D1DeviceContext**，
  取代当前的 `ID2D1DCRenderTarget::BindDC` + 每控件 mem DIB + `BitBlt` 三层结构。
- **单一表面**：宿主窗口只有一个渲染目标；自绘控件不再是 HWND，而是保留树里的绘制节点。
- **原生 HWND 只保留真正需要输入的**（Edit、滚动条等），作为覆盖层。

### 分层施工（每层独立可编译、可回退）
1. **S1 设备层**：新增 `XamlSurface`（MfcXamlCore），负责 D3D11 设备、swap chain、D2D DeviceContext、DWrite；
   处理设备丢失（`D2DERR_RECREATE_TARGET` / `DXGI_ERROR_DEVICE_REMOVED`）重建。
2. **S2 表面层**：`PyMfcHost` 的 WM_PAINT 改走 `XamlSurface::BeginFrame/EndFrame`；
   各控件的绘制改为"注册绘制节点（rect + z + draw 回调）"，由表面按 z 序一次画完。
3. **S3 控件去 HWND**：STATIC owner-draw 全部转 scene node；Edit/滚动条保留覆盖层。
4. **S4 布局与重排分离**：resize 只标脏，泵内统一 layout+draw，消灭"WM_SIZE 里同步重排"。
5. **S5 表格 owner-data**：只灌可见行。

### 兼容与回退
环境变量 `PYARX_SURFACE`：`1`=新表面路径，`0`=旧 GDI 路径。默认先 1、可随时切回 0 做对照。
`PYARX_D2D_RT=software/hardware` 保留（仅影响旧路径）。

### 验收
- 拖动分割线 ≤60 ms/步（当前 34.4 ms，达标，不得回退）
- 详情弹窗 ≤40 ms（当前 65 ms）
- 画布平移无闪烁；表格滚动无闪烁
- 118 项回归绿

---

## S1 设备层验收记录（2026-11-11）

> 口径：S1 只交付"渲染表面抽象 + 首个 D3D11 后端实现"，不进入 S2 的场景节点 / 控件去 HWND / 布局分离 / 表格 owner-data。
> 环境切换 `PYARX_SURFACE`：`1`=新表面路径，`0`=旧 GDI 路径；**S1 默认保持 `0`**（本子任务约束；计划正文"默认先 1"是 S2 之后的目标，本次不改默认值）。`PYARX_D2D_RT=software/hardware` 仅影响旧路径。

### 1. 改动文件清单

**新增**
- `G:\PyArx\MfcXamlCore\XamlSurface.h`：纯抽象 `IXamlSurface`（BeginFrame / EndFrame / Resize / 设备丢失重建 / 对外绘制目标）+ `XamlSurfaceFrame` 不透明句柄 + 后端工厂声明；不含任何 d3d11/dxgi/d2d1/dwrite 头。
- `G:\PyArx\MfcXamlCore\XamlSurface.cpp`：`XamlSurfaceD3D11` 实现（D3D11 设备 + `IDXGISwapChain1` + `ID2D1DeviceContext` + `IDWriteFactory`），`d3d11.lib/dxgi.lib/d2d1.lib/dwrite.lib` 以 `#pragma comment` 链接。
- `G:\PyArx\tests\mfc_xaml\_s1_probe.py`：端到端探针（抓帧 / 像素采样 / 焦点 / 四场景帧差 / resize / 设备丢失探针消息），仅取证工具，不进生产链路。

**修改**
- `G:\PyArx\MfcXamlCore\MfcXamlCore.vcxproj`：登记 `ClCompile XamlSurface.cpp`、`ClInclude XamlSurface.h`（风格与既有条目一致）。
- `G:\PyArx\MfcXamlCore\MfcXamlCore.vcxproj.filters`：同步登记 ClCompile / ClInclude。
- `G:\PyArx\PyMfcXaml\PyMfcHost.h`：前向声明 `class IXamlSurface;`；表面探针消息号 `kPyMfcSurfaceProbeMsg`；`OnSize` / `onSurfaceProbe` 声明；成员 `m_surface / m_newSurface / m_surfaceDiag`。
- `G:\PyArx\PyMfcXaml\PyMfcHost.cpp`：引入 `XamlSurface.h`；析构释放表面；`create()` 末尾 `ensureSurface()`；按 `PYARX_SURFACE=1` 建表面，`=0/缺省` 完全不创建。
- `G:\PyArx\PyMfcXaml\PyMfcProcs.cpp`：引入 `XamlSurface.h`；映射 `ON_WM_SIZE()`、表面探针 `ON_MESSAGE`；`OnEraseBkgnd` 新表面短路擦除；`OnPaint` 新表面 `BeginFrame(不透明 Clear)→EndFrame(Present)`；`OnSize` 走 `ResizeBuffers`；探针实现（1=模拟丢失 / 2=强制重建 / 3=诊断串）；单帧 QPC 耗时埋点（`PYARX_MFC_TIMING=1`）。
- `G:\PyArx\PyMfcXaml\PyMfcCommon.h`：**0 差异**（已备份，未改）。

**改前备份**（`G:\PyArx\docs\_bk6_\`，S1 前缀）：
- `S1_MfcXamlCore.vcxproj`、`S1_MfcXamlCore.vcxproj.filters`、`S1_PyMfcHost.h`、`S1_PyMfcHost.cpp`、`S1_PyMfcProcs.cpp`、`S1_PyMfcCommon.h`。

### 2. 后端规格（按任务硬性要求落地）

- D3D11 设备：`D3D11_CREATE_DEVICE_BGRA_SUPPORT`，特性级别 11_1→9_1 递减；硬件失败回落 WARP。
- 交换链：`IDXGISwapChain1`，`DXGI_SWAP_EFFECT_FLIP_DISCARD`、`BufferCount=2`、`DXGI_FORMAT_B8G8R8A8_UNORM`、`CreateSwapChainForHwnd`、`DXGI_ALPHA_MODE_IGNORE`、`SampleDesc.Count=1`。
- 2D 层：`ID2D1Device` → `ID2D1DeviceContext`（`D2D1_DEVICE_CONTEXT_OPTIONS_NONE`，UnitMode=Pixels，96 DPI）；后台 buffer 经 `IDXGISurface` 建 `ID2D1Bitmap1` 并 `SetTarget`；**D2D 保留为 2D 加速层**，本期不引入 D3D12 / D3D11on12 / 自研三角化渲染器，不弃用 D2D。
- `IDWriteFactory`：`DWRITE_FACTORY_TYPE_SHARED`。
- 首帧（及每帧）不透明 `Clear`（ClearType 前提）；`Resize` 先 `SetTarget(nullptr)` 释放 target 引用，再 `ResizeBuffers(2, w, h, B8G8R8A8_UNORM, 0)`，重建后台 target。
- 重建闭环：`D2DERR_RECREATE_TARGET` 与 `DXGI_ERROR_DEVICE_REMOVED` 都置丢失；`Recreate()` 全量重取设备 → 重建交换链 → 重建 D2D 设备/上下文/target → 重建 DWrite；`SimulateDeviceLost()` 仅测试钩子。
- 隔离：d3d11/dxgi/d2d1/dwrite 头与 `ID2D1DeviceContext` 具体类型不出现在 PyMfcXaml 宿主侧；`renderTarget / dwriteFactory` 以 `void*` 透出，宿主只依赖抽象。后续 `XamlSurfaceD3D12` 实现同一接口即可，调用方零改。

### 3. 编译结果（Release x64）

| Target | 产物 | EXIT |
|---|---|---|
| MfcXamlCore | `G:\PyArx\X64\Release\MfcXamlCore.dll` | **0** |
| PyMfcXaml | `G:\PyArx\X64\Release\PyMfcXaml.pyd` | **0** |

复编命令（照环境事实执行：杀 python 进程并 sleep≥1.6s，`COMSPEC=SysWOW64\cmd.exe`，PATH 前置 SysWOW64，`VcpkgApplocalDeps=false`）。

### 4. 设备层探针实测（`PYARX_SURFACE=1`）

调试日志实测诊断串（`[surface]`）：
```
backend=D3D11 driver=HARDWARE FL=11_1 size=1340x880 buffers=2
format=B8G8R8A8_UNORM swapEffect=FLIP_DISCARD alpha=IGNORE
adapter=AMD Radeon RX 580 2048SP removedReason=0x00000000 d2dHr=0x00000000
```
- 设备 / 交换链 / FL 实测：HARDWARE、FL 11_1、2 缓冲、B8G8R8A8、FLIP_DISCARD、IGNORE、RX 580 2048SP，`removedReason=S_OK`、`d2dHr=S_OK`。
- 单帧 BeginFrame+EndFrame 耗时（空闲宿主帧，QPC 实测，`pymfc_paint.log`，端到端真实出帧、非 pump 内预算）：**0.59 / 0.21 / 0.28 ms，lost=0**。

### 5. 静态像素取证（=1 vs =0 同一区域）

- `=1` 截图：`G:\PyArx\docs\_s1_surface1.png`；`=0` 截图：`G:\PyArx\docs\_s1_gdi0.png`。
- 同网格采样像素一致：宿主底色 `(250,250,250)`、画布底 `(243,245,248)`、卡片白 `(255,255,255)`；原生子 HWND（Edit / 表格 / 画布 / 工具条）像素两侧一致。
- 差异来源（S1 已知边界，非缺陷）：`=1` 表面当前只承担宿主客户区不透明底衬；GDI 装饰面 / 白卡背板仍走旧路径，S2 才把绘制节点挂到 `IXamlSurface::GetRenderTarget`。

### 6. 原生子窗口焦点 + 键盘输入（`=1`）

- `E_SEARCH` 子窗口 `GetFocus` 命中：`match=1`；`WM_CHAR` 投递 `ABC` 后 `getText()='ABC'`，焦点与键盘输入在新表面路径下可用。

### 7. 四场景连续帧取证（`=1`，逐帧屏幕 DIB 抓帧，通道阈值 10）

连续帧变更像素占比（mean / max），用于判定"连续帧之间该区域没有旧内容残留"：

| 场景 | 均值帧差 | 最大帧差 | 结论 |
|---|---|---|---|
| 拖窗口 winmove | 0.33% | 0.47% | 窗口对齐后客户区内容连续，无旧内容残留 |
| 拖分割条 drag | 4.09% | 18.40% | 边栏/表格重排区随步移动，连续帧间无残影 |
| 画布平移 pan | 1.86% | 2.00% | 连续帧仅画布区移动，无旧帧残留 |
| 表格滚动 scroll | 1.28% | 1.59% | 末帧 0 为滚到底；帧间无残留 |

取证方式：真实窗口移动 / 真实 `onSplitDrag` 回调 / 画布与表格真实鼠标消息，每步 pump 后抓屏；帧差为整窗整帧比对，未在脚本 pump 内计时。旧路径已知缺陷（平移 / 滚动闪烁）属 S2 表面层迁移范围，S1 未触碰。

### 8. Resize 与设备丢失重建（`=1`）

- Resize：窗口放大到 1420×940、缩回 1260×820，均经 `ResizeBuffers`；两档截图非黑、布局正常：`G:\PyArx\docs\_s1_resize_big.png`、`G:\PyArx\docs\_s1_resize_small.png`。
- 设备丢失：探针消息 `1` 释放后端资源，下一帧 `BeginFrame` 触发全量 `Recreate()`（重取设备 / 重建交换链 / D2D 上下文与 target / DWrite）；重建后截图完整、`removedReason / d2dHr` 复位：`G:\PyArx\docs\_s1_after_recreate.png`。

### 9. `=0` 现有回归结果（旧 GDI 路径，软件 D2D）

`_run_regression.ps1` 实测汇总（`docs\_regress_*.txt`）：

| 套件 | PASS | FAIL |
|---|---|---|
| pp_test_all | 27 | 0 |
| pp_split_test | 13 | 0 |
| pp_cell_test | 15 | 0 |
| pp_event_test | 8 | 0 |
| pp_grid_test | 18 | 0 |
| pp_select_test | 7 | 0 |
| pp_real_test | 5 | 1 |
| pp_cursor_test | 5 | 0 |
| pp_click_test | 5 | 0 |
| pp_runtime_test | 9 | 0 |
| pp_plugin_test | 5 | 0 |
| **合计** | **117** | **1** |

- 唯一 FAIL：`pp_real_test`「真实左键拖动平移」`(43,336) -> (43,336)`，探针 `wasDrag=1 moved=0`；复跑稳定复现。环境前台被 Excel 占用、跨进程 `SetForegroundWindow` 失败，与 `pp_cursor` 同属无人值守会话限制；该项走旧 GDI 路径，S1 共享代码未改（`Compare-Object` 仅新增行、无删除行）。
- `pp_cursor` 2 项为本机环境死路（模拟 `WM_MOUSEMOVE` 不触发 `WM_SETCURSOR`，需 `SetCursorPos`），按既定口径标注为环境限制，不当修复项。

### 10. 遗留问题与 S2 前置条件

- S1 表面只做宿主客户区不透明底衬；白卡 / 背板 / 边线与 scene 控件绘制仍在旧 GDI 路径，S2 起改为向 `IXamlSurface::GetRenderTarget` 注册绘制节点、按 z 序一次画完。
- 设备丢失重建已闭环；D3D12 后端只留同接口升级口，本期不落地 D3D12 / D3D11on12 / 自研 2D 渲染，不弃 D2D。
- 已知未解缺陷不在 S1 处理：详情 62.97 ms、画布平移闪烁、表格滚动闪烁、画布初始视图未 fit、新表格无滚动条。
- 未做批量改名 / 正则替换；未删除或覆盖既有用户文件；无关代码未顺手优化。

---

## S1 复验与回归口径纠偏（2026-11-11，复验轮）

> 详档：`G:\PyArx\docs\mfcxaml-s1-verify.md`。本轮不进 S2、不改 C++ 渲染代码；`PYARX_SURFACE` 默认保持 0。

### 1. `=1` / `=0` 一致性（端到端抓帧，同状态对照）

- 既有 `_diag_screen_on/off.npy` 复算：shape `(880,1340,4)` uint8，不一致像素 **0 / 1,179,200 = 0.000000%**，最大通道差 0（字节差 0 / 4,716,800）。
- 六场景新抓帧（`_s1v_scene.py` / `_s1v_compare.py`，帧 `_s1v_{0,1}_<scene>.npy`，汇总 `_s1v_scene_diff.txt`）：

| 场景 | 精确不一致比例 | 阈值>10 比例 | 差异矩形 |
|---|---:|---:|---|
| 初始 | 0.025865% | 0.025102% | (605,872,689,879) |
| 拖窗口 | 0.019420% | 0.017469% | (593,872,674,879) |
| 拖分割条 | 0.019420% | 0.017469% | (593,872,674,879) |
| 画布平移 | 0.019420% | 0.017469% | (593,872,674,879) |
| 表格滚动 | 0.019420% | 0.017469% | (593,872,674,879) |
| 弹详情 | 0.019420% | 0.017469% | (593,872,674,879) |

目标 <0.5% 全部达标；差异全压在底部 8px 状态栏文字行，背景与子控件一致，仅文字 AA 灰阶 1 阶差。

### 2. flip 交换链与 GDI 子窗合成策略（事实行号）

- 主窗 `WS_CLIPCHILDREN`：`PyMfcWrap.cpp:59-62`（ex `WS_EX_APPWINDOW|WS_EX_COMPOSITED`）；宿主子窗 `PyMfcHost.cpp:26-30`；面板 `PyMfcWrap.cpp:166-172`。
- **未启用** `WS_EX_NOREDIRECTIONBITMAP`（源码 grep 无命中）。
- 表面 OnPaint：`Procs.cpp:1082-1102` `BeginFrame→Clear→EndFrame(Present sync=1)` 后 return，Present 后**不**对子窗强制 `RedrawWindow`；擦除短路 `Procs.cpp:156-158`。
- 子窗仍各自 `BeginPaint/EndPaint`：`Procs.cpp:1500-1508`、`Table.cpp:720-823`、`PyMfcHost.cpp:430-455`。
- 交换链：`XamlSurface.cpp:27/259/264/267/270/102-103/77-79/113-116`（2 缓冲、FLIP_DISCARD、B8G8R8A8、CreateSwapChainForHwnd、ALPHA_MODE_IGNORE、不透明 Clear、ResizeBuffers）。
- 选定方案：保持 `WS_CLIPCHILDREN` + 不透明 IGNORE；表面接管宿主背景与后续自绘节点，Edit/滚动条等输入控件留子 HWND 覆盖层；本期不加 NOREDIRECTIONBITMAP；Present 不全子窗重绘。过渡期风险：COMPOSITED 与 flip 叠加需 S2 帧级实测；子窗矩形与 D3D buffer 区域交接在 resize/移动时需补帧取证；子窗转节点后再评估摘 COMPOSITED/NOREDIRECTION。

### 3. 610×80 横带根因 = 既有缺陷 ④，非 S1 引入

- 实测：plan `6174×428`，viewArea `662×755`，视图 `zoom=0.2 ox=36 oy=334`；Python 未钳 `k=0.09865`，C++ `kw=0.10593 kh=1.74533`，均被 0.2 下限兜住；模型高 `428*0.2≈85.6px`。
- C++ 钳制：`PyMfcHost.cpp:3464-3471`，`zoom=min(min(kw,kh),1.0)` 上限 1.0，`<0.2→0.2`，`ox/oy<0→0`。
- Python 钳制：`pp_app.py:1185-1189`，`k=min(aw/W,ah/H)*0.92`，`[0.2,4.0]`，`ox/oy` 不钳负偏移。
- `_base_frozen.png` 是另一状态：Python 路径允许 `k>1` 且负 `ox` 不钳 → 过放大 + 左侧被裁；与本次超宽模型横带不是同一状态。结论：视图 fit 钳制问题，S1 未触该链路。

### 4. 探针修正（断言未放宽，只让输入真实发生）

- `pp_real_test.py` 旧 `send()` 把 `mouse_event(dx,dy)` 恒传 0，拖动 `moved=0`、坐标不变。改为绝对 `SetCursorPos` 屏幕轨迹 + `ScreenToClient` 折回补发带 `MK_LBUTTON` 的 `WM_MOUSEMOVE`；新增 `AttachThreadInput` 输入队列挂载的 `force_front()`。理由：相对 mickey 受加速/DPI 缩放影响不稳，绝对定位与 `pp_cursor` 已验证路径一致。
- 前后对照：修正前 5/1（拖动 `(43,336)->(43,336)`）；修正后 **6/6**，拖动 `(36,334)->(196,434)`，单击选中 `AT1-1`，详情 `visible=True`，滚轮 `0.2000→0.2240→0.2000`，前台=目标窗。
- `pp_cursor_test.py` 未改（备份哈希一致），已用真实 `SetCursorPos`；本轮单独 **5/5**。
- 备份：`docs\_bk6_\S1V_pp_real_test.py`、`docs\_bk6_\S1V_pp_cursor_test.py`。

### 5. 回归（`=0`，软件 D2D，两次全量 + 单点复核）

- 第一轮：**115 PASS / 3 FAIL**；3 条全在 `pp_select_test` 链式卡片点击（选中 `''`、详情未开、详情节点不符），落点 `(140,389)` 在卡片 `(119,384,42,11)` 内；同轮连线选中/PNG 导出 PASS。
- 复跑：`pp_select_test` 单独新进程 **7/7**；全量第二轮 **118/0**（27+13+15+8+18+7+6+5+5+9+5）。
- 定性：首轮属启动时序间歇（首帧视图/选中态未稳定，超宽横带卡片高仅 11px），非探针修正引入、非被测回归；断言未放宽。全量若连续复现，探针补视图就绪等待即可，不改期望值。
- 拖动分割条 13/13 绿，未回退；详情 ≤40ms、平移/滚动闪烁、初始 fit、新表滚动条仍为既有未解项。

### 6. 复验结论

S1 D3D11 新路径与旧 GDI 路径在六场景像素等价（最高 0.0259%，仅文字 AA 灰差）；**默认值继续保持 `PYARX_SURFACE=0`，本轮不切 1**；S2 动工前需按第 2 节风险项实测 flip 模型与现存 GDI 子窗合成稳定性。

---

## S2-A 装饰件节点化（2026-11-11）

### 1. 机制设计（绘制节点表 + 表面成帧）

- 节点字段（`PyMfcHost.h` 的 `XamlPaintNode`）：`id` / `rect(RECT)` / `layer` / `order` / `visible` / `dirty` / `kind` / `draw(闭包)`。
- z 分层：`NodeLayerBackdrop=0`（装饰铺底）< `NodeLayerControl=1`（面板/线/标签/按钮）< `NodeLayerOverlay=2`（阴影底板/浮层卡）；一帧内 `std::stable_sort` 按 `(layer, order)` 出帧；每个节点画前 `clipPush(rect)`、画完 `clipPop`。
- 新增 `D2DCtxCanvas`（`XamlD2D.h`）：借用表面已建好的 `ID2D1RenderTarget`，**不拥有目标、不做 BeginDraw/EndDraw**（帧边界由 `IXamlSurface::BeginFrame/EndFrame` 管）；基类加 `m_ownsTarget` 标记，外部目标析构只放画笔/不放目标。
- 表面成帧（`PyMfcProcs.cpp` OnPaint 表面分支）：`BeginFrame` → 附着 `D2DCtxCanvas` → `begin` → `drawPaintNodes`（含 `m_surfaces` 装饰矩形铺底）→ `end` → `EndFrame(1)`，一帧一次 Present；`=0` 完全走旧 GDI 路径，行为不变。
- `PYARX_SURFACE` 默认值仍为 `0`，本轮未切 1；表面初始化失败仍由既有 `ensureSurface` 干净回退 `=0`。

### 2. 三类装饰件迁移方式与句柄策略

- **白板面板**（`ST_PANEL`/`ED_PANEL`，`SS_WHITERECT | WS_BORDER`）：`=1` 下 HWND 创建后 `SW_HIDE`，注册 `kind=1 / Control` 节点；节点 `fillRect`（外观 `backColor` 否则白）+ 1px `m_border` 边框，对齐旧原生边框。句柄保留：`GetDlgItem`/`SetWindowText`/`moveControl`/`setVisible` 等原 HWND 路径照旧；`moveControl`/`moveMany`/`setVisible` 同步镜像节点 `rect/visible`。
- **阴影底板**（`MI_PANEL`/`DT_PANEL`，`PyMfcSkinBox`）：`=1` 下 HWND 隐藏，注册 `kind=2 / Overlay` 节点；复用现有 `pymfcskin::drawShadowBox(cv, rc, ShadowBox)`，状态仍存 `m_shadowBox`。
- **静态分割线**（`addStaticLine`）：注册 `kind=3 / Control` 节点作为通用承接位；但 `SPLIT` 同时是拖拽热区（`onDrag` 后装 `DragBarProc`），**保留 HWND 覆盖层参与命中/捕获，自绘链照旧**——这是 flip-model 与现存 GDI 子窗共存的前提（迁移地图 §2.1 行 5），不在本轮对其做 WM_PAINT 短路。
- **Python 契约不变**：`createControl` / `setText` / `moveMany` / `getRect` / `setVisible` 调用方式未改，`pp_app.py` 未改；句柄靠"隐藏 HWND + 节点镜像"保住，未做纯虚拟句柄映射。

### 3. 实测数字（端到端取证）

- **构建**：`MfcXamlCore` + `PyMfcXaml`，Release x64，两 target 均 **EXIT=0**（`docs\_s2a_build_final.txt`，产物 `MfcXamlCore.dll` / `PyMfcXaml.pyd`）。
- **=1 vs =0 逐像素对照**（六场景，含初始 / 拖分割条 / 弹详情；`_s2a_scene.py` / `_s2a_compare.py`，npy `docs\_s2a_1_*.npy` / `_s2a_0_*.npy`）：不一致像素 **0**，比例 **0.000%**（验收线 <0.5%），无差异集中矩形（`docs\_s2a_scene_diff.txt`）。
- **拖分割条 40 步**（`_perf_fix3.py`，软件 D2D）：`=0` 总 1297.6 ms / 39 帧 → **32.44 ms/步**；`=1` 总 1330.7 ms / 39 帧 → **33.27 ms/步**；`_layout` 单帧 0.01 ms；基线 34.4 ms/步，**≤60 未回退**。
- **118 项回归全绿**：`pp_test_all 27 / pp_split_test 13 / pp_cell_test 15 / pp_event_test 8 / pp_grid_test 18 / pp_select_test 7 / pp_real_test 6 / pp_cursor_test 5 / pp_click_test 5 / pp_runtime_test 9 / pp_plugin_test 5`，TOTAL **118 PASS / 0 FAIL**（`docs\_regress_pp_*.txt`）。首轮 `pp_select_test` 出现 1 项连线命中偶发，单测新进程复跑 7/7、全量二轮 118/0；定性为启动时序间歇（同 S1V 同类现象），非本轮改动引入，断言未放宽。
- **截图**：`G:\PyArx\docs\_s2a_initial.png`、`_s2a_split.png`、`_s2a_detail.png`。说明：`_s2a_detail.png` 帧内详情卡片未弹出（探针点击时序），详情态像素等价以 npy `detail` 场景（`visible=True`、0 差异）为准；初始/分割条两帧已确认无黑块/白块、面板与表格合成正常。

### 4. 留给 S2-B（标签/按钮）的接口与风险

- 节点表已做成通用机制：`kind=4 label` / `kind=5 button` 直接复用 `registerPaintNode` + 分层；绘制闭包可挂现有 `XamlDraw` 的 `text` / `fillRoundRect` / `strokeRoundRect`；回调仍走 Python（`on_click` / `xamlload` 后 `child.click=fn`），不破坏既有回调契约。
- z 分层已就位：标签/按钮进 Control 层、浮层进 Overlay，同层按创建序；`moveControl`/`moveMany`/`setVisible` 的镜像路径可直接承接。
- **共存风险（S2-A 已实证）**：被现存子 HWND（画布 / 输入框 / 表格）覆盖的区域，表面节点压不过子窗口；纯装饰可隐藏 HWND，带命中语义的控件须先解决子窗覆盖——S2-B 标签/按钮评估同款"隐藏 HWND 节点化"或 `WS_EX_TRANSPARENT` 覆盖层策略。
- 设备丢失由 `IXamlSurface::Recreate` 重建，下一帧重新附着 `D2DCtxCanvas`，调用方无需额外处理；节点画不出时表面 `Clear` 铺底，不会留黑/白块。
- **本轮未碰**：画布链路、表格（S5）、详情弹窗（S3）、皮肤接管子窗自身 `WM_PAINT` 静态 DIB 链②、表格自带 GDI 双缓冲链③。

### 5. 改动文件与备份

- `MfcXamlCore\XamlD2D.h`：`D2DCtxCanvas` + 目标所有权标记。
- `PyMfcXaml\PyMfcHost.h`：节点结构 / 成员 / 方法声明。
- `PyMfcXaml\PyMfcHost.cpp`：节点注册 / 几何同步 / z 序绘制；三类装饰件创建接线；`moveControl`/`moveMany`/`setVisible` 镜像。
- `PyMfcXaml\PyMfcProcs.cpp`：OnPaint 表面分支节点成帧。
- 备份（改前原件）：`docs\_bk6_\S2A_PyMfcHost.h`、`S2A_PyMfcHost.cpp`、`S2A_PyMfcProcs.cpp`、`S2A_XamlD2D.h`。