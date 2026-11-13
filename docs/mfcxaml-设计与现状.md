# MFCXaml 设计与现状

> 本文是 `MfcXamlCore` / `PyMfcXaml` 的**唯一总文档**：架构规矩、控件封装现状、绘制架构阶段状态、性能基线、缺陷台账、回归口径、构建环境。
> 2026-11-13 合并 `mfcxaml-render-refactor-plan.md`、`mfcxaml-s1-verify.md`、`mfcxaml-s2-migration-map.md`、`mfcxaml-s3-spec.md`、`mfcxaml-s4-s5-spec.md`、`session-2026-11-11-mfcxaml-digest.md` 六份过程文档而成；原始长文留在 git 历史（`v0.10.8` 及更早标签）。
> 过程中被推翻的旧结论一律不写进来；留下的都是复核过的现状与待办。

## 一、架构规矩（最高优先级）

1. **MFCXaml 只提供控件库**：`Button/Label/TextBox/ListCtrl/ListBox/Panel/StaticLine/StaticBitmap` 等基础控件；不在 C++ 里为业务界面写布局，也不在 Python 里算像素。
2. **复合控件用 XAML 组合基础控件**，放 `G:\PyArx\plugins`：一个 `<Window Name="...">` 文件 = 一个复合控件，`x:Name` 是子控件名，几何来自 `M.xamlLayout(path, w, h)`。
3. **通用组合可下沉成 C++ 控件类**，接口不变（XAML 标签名、属性、`on_*` 回调名都不改，只把创建与事件搬进 C++）；判定标准见 `plugins\README.md`。
4. **所有回调一律回 Python**，两种写法等价可混用：
   - XAML 预声明：`<Button Name="BtnApply" Text="应用" on_click="on_apply"/>`，加载时按名字在调用方作用域解析；
   - 运行期赋值：`btn = xamlload(path); btn.BtnApply.click = fn`。
   事件名：`click / dblclick / change / grid`（`grid` 回调签名 `(action, row, col, text)`）。
5. **布局大小交给 XAML**；Python 侧仍存的手算坐标属未完成项，不是规范。

## 二、模块分层与关键文件

| 层 | 位置 | 职责 |
|---|---|---|
| 控件库/解析 | `MfcXamlCore\`（`XamlCore.cpp`、`controls\Xaml*.h/.cpp`、`XamlDraw.h`、`XamlD2D.h`、`XamlSurface.h`、`PyMfcSkin.h`） | XAML 解析、布局、样式、模板、绑定表达式、自绘控件、D2D/GDI 绘制原语、表面层 |
| MFC 宿主 + 绑定 | `PyMfcXaml\`（`PyMfcHost.*`、`PyMfcProcs.*`、`PyMfcLoad.*`、`PyMfcWrap.h`、`PyMfcBind.cpp`） | 窗口、消息泵、皮肤自绘、事件派发、`addXxx/setXxx` 级 API、`prop_schema()` |
| 复合控件框架 | `MfcXamlRuntime\`（Python） | Composite / Runtime / 自动事件绑定与属性映射 |
| 复合控件实例 | `plugins\`（`side_panel.xaml/.py`、`README.md`） | 用 XAML 组合出的业务面板，含"何时下沉 C++"判定 |
| 回归 | `tests\mfc_xaml\`（`pp_*.py`、`_smoke_proptable.py`、`_sim_msgs.py`、`_audit_*.py`、`_run_regression.ps1`） | 模拟消息/真实点击用例、属性表冒烟、契约与标签审计 |

## 三、控件封装现状（已完成）

- **属性统一走声明式属性表** `XamlPropEntry{Name, Aliases, Type, EnumValues, Set, Get}`（`controls\XamlTypes.h`）：名字、类型、只读/只写、读写实现只在一处定义，基类 `properties()` 统一分派。
- **覆盖 39 个控件**：Button、TextBlock、CheckBox、RadioButton、GroupBox、Switch、ListBox、Choice、ListCtrl、TabControl、TreeCtrl、Grid、ListView、TreeView、GridView、PathView、PageIndicator、TextBox、ComboBox、TableView、Slider、ProgressBar、SpinBox、Dial、Tumbler、ScrollBar、RangeSlider、Menu、MenuBar、ToolBar、ToolTip、ToggleButton、Hyperlink、RichText、CodeEditor、ScrolledWindow、DatePicker、QCanvas、PowerCanvas、Popup、PageView、StaticBitmap、BitmapButton，以及无自有属性的 StaticLine/Splitter/Panel/Flickable/BusyIndicator/CalendarCtrl。
- **迁移契约**：值在 attach 前缓存到成员、在 `createNative` 里下发；`Set == nullptr` 即只读；`Name` 不作为自有属性（基类 `IsLayoutAttribute` 已处理）。
- **schema 导出**：`XamlPropertySchemaJson()`（`MfcXamlCore\XamlCore.h/.cpp`）+ Python `PyMfcXaml.prop_schema()`，结构 `{"version":1,"controls":[{"tag":"Button","properties":[{"name":"Text","type":"String","readOnly":false,"enum":""}]}]}`；实测 51 个控件、77137 字节。
- **事件**：`XamlEvents.h` 契约表，XAML 只写事件名，控件申报 `events()`，宿主按契约绑定派发。
- **只读属性槽位缺陷已修**：`properties()` 曾把 `writable` 填进 `readable` 槽位，导致 TableView.RowCount、Menu.SelectedIndex 等在 schema 里显示为可写；现为 `{e.Name, e.Type, true, writable, e.EnumValues}`。
- 迁移审计脚本 `tests\mfc_xaml\_audit_prop_migration.py` 当前输出 **0 个控件待迁移、0 项属性**。

## 四、绘制架构现状

### 4.1 现存三条绘制路径（改造前基线）

1. owner-draw 子窗口（STATIC / BS_OWNERDRAW）经父窗口 `WM_DRAWITEM` 直绘；
2. 皮肤接管子窗口经自身 `WM_PAINT` + 离屏 DIB 双缓冲；
3. 自注册 `PyMfcTable` 自带 GDI 双缓冲。

问题根因：**每个控件一个 HWND、各自 `WM_PAINT`**，一次操作要重画多张表面（侧板一块纯色矩形就要 16.6 ms 即为证据）。

### 4.2 目标态与阶段进度

目标：**父表面一次绘制 + 少量真 HWND 只负责输入**（单一表面、保留节点树、控件不再是 HWND），路径由环境变量 `PYARX_SURFACE` 切换：`1`=新表面路径、`0`=旧 GDI 路径。

| 阶段 | 内容 | 状态 |
|---|---|---|
| S1 设备层 | `XamlSurface`：D3D11 flip 交换链成帧、设备丢失重建、resize、软件/硬件 D2D 后端；与旧路径六场景像素等价（最大差 0.0259%，仅文字 AA） | **已完成**（默认值仍 `0`） |
| S2-A 装饰件节点化 | 纯装饰件迁移为表面绘制节点（绘制节点表 + 单次成帧），三类装饰件句柄策略已落地 | **已完成** |
| S2-B | 标签 / 按钮等带命中语义控件节点化（"隐藏 HWND 节点化"或 `WS_EX_TRANSPARENT` 覆盖层） | **未开始** |
| S3 | 详情卡底板 + 阴影并入画布 D2D 层；④初始视图 fit 补丁；⑤滚动条缺口 | **仅规格，未施工** |
| S4 | resize 只置脏、泵内统一 layout+draw（布局与重排分离） | **仅规格，未施工** |
| S5 | 表格 owner-data 虚拟列表（只灌可见行）+ ③闪烁 + ⑤滚动条 | **仅规格，未施工** |

已知约束（S2-A 已实证）：被现存子 HWND（画布 / 输入框 / 表格）覆盖的区域，表面节点压不过子窗口；带命中语义的控件必须先解决子窗覆盖，再谈节点化。

## 五、性能基线（软件 D2D，39 节点 / 39 行 / 8 列，中位数）

| 操作 | 数值 |
|---|---|
| `pp_app._layout()`（40 个 `moveMany`） | 1.8 ms（旧 2.1） |
| `fill_table()`（`setTableRows` 批量灌 39×8） | 15.6 ms（最快 9.7；旧逐行插入 22.5） |
| `collect_grid()` / `setTableSelectedRow` 单次 | 0.2 ms / 0.0 ms |
| 逐行选中 39 行 | 129 ms（3.3 ms/行，含回调 + 画布选中 + 详情） |
| 强制重绘：画布 / 表格 / 侧板白卡 / 状态栏 / 小地图 | 6.0 / 8.7 / 16.6 / 1.7 / 0.0 ms |
| **强制重绘整窗** | **82.1 ms（≈12 fps）** |
| 详情弹窗（口径 B，真实点击到探点像素出现） | 中位 **62.97 ms**、均值 63.59 ms（目标 ≤40，**未达标**） |

结论：单控件不慢，瓶颈是"一次操作重画多少张面"与"一次数据变化跨多少接口"；`fill_table` 再降一个量级只能靠 S5 的 owner-data（数据只在控件里、按可见行画）。

## 六、缺陷台账

| 编号 | 现象 | 现状 |
|---|---|---|
| ① | owner-draw 子窗经父 `WM_DRAWITEM` 直绘 | 结构性问题，随节点化逐步消解 |
| ② | 画布平移闪烁（皮肤接管子窗自身 `WM_PAINT` + 离屏 DIB） | 未修；S3/S5 节点单表面后应自然消失，待实测判据 |
| ③ | 表格滚动闪烁（`PyMfcTable` 自带 GDI 双缓冲） | 未修；同上，随 S5 处理 |
| ④ | 画布初始视图未 fit（整图放大、左侧被裁） | 未修；根因是 C++ 封顶 1.0/钳 ≥0 与 Python 上限 4.0/未钳 ≥0 两套 fit 规则漂移 |
| ⑤ | 新表格"无滚动条/拖不动" | 未修；需先区分"拇指不出"与"拖不动"两种情形，再决定拖拽补丁；水平滚动是否需要按列宽算术判定 |

## 七、回归与验收口径

- 全量自动测试 **118 项**：`pp_test_all` 27、`pp_split_test` 13、`pp_cell_test` 15、`pp_event_test` 8、`pp_grid_test` 18、`pp_select_test` 7、`pp_real_test` 6、`pp_cursor_test` 5、`pp_click_test` 5、`pp_runtime_test` 9、`pp_plugin_test` 5。
- 批次脚本 `_run_regression.ps1`：每个 GUI 用例前杀 python 进程并 sleep ≥1.4 s（残留窗口抢焦点会造成假失败），连跑两个 GUI 用例之间同样要杀进程 + sleep。
- 断言只允许"让输入真实发生"（真鼠标 `SetCursorPos` + `GetCursor`），不允许放宽断言；`_smoke_proptable.py`（属性表冒烟）与 `_sim_msgs.py`（模拟消息）结果要求均为 0 项失败。
- 审计：`_audit_prop_migration.py`（属性表迁移完整性，应为 0）、`_audit_contract_keys.py`（事件契约键）、`_audit_tags.py`（注册标签）；改名工具 `_unify_props.py` / `_unify_usage.py`。

## 八、构建与环境

- Python：`D:\Program Files\Python314\python.exe`（3.14.8）；PATH 上的 3.13 加载不了 pyd。
- ObjectArx：`G:\ObjectArx\2024`（inc / inc-x64 / utils\brep / lib-x64）。
- 构建（`VcpkgApplocalDeps=false` 必带，否则失败）：
  ```
  MSBuild G:\PyArx\PyArx.sln /t:PyMfcXaml /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false /m /nologo /v:m
  ```
  `ComSpec` 用 `System32\cmd.exe`（`SysWOW64` 版对 UIDesign 报提权失败）。编前关掉占用 pyd 的 python，否则 LNK1104。
- 跑测：`$env:PYARX_D2D_RT='software'`、`$env:PYTHONIOENCODING='utf-8'`，`cd G:\PyArx\tests\mfc_xaml`；产物在 `X64\Release`（`PyMfcXaml.pyd`、`MfcXamlCore.dll`、`PyArxCore.pyd`）。
- 版本：`master` 为最新代码，每次提交由 `tools\git_tag_version.ps1` 打 `vX.Y.Z` 标签；历史版本用标签回看。

## 九、待办（按影响排序）

1. **S2-B**：标签/按钮节点化，先解决"子 HWND 覆盖表面节点"的共存策略。
2. **S3**：详情卡 62.97 → ≤40 ms（底板与阴影并入画布层，把多次 `BeginPaint` 合成降到一次 `Present`）。
3. **⑤/④**：新表格滚动条（先诊断再补丁）、画布初始视图 fit（统一 C++ 与 Python 两套规则）。
4. **S4**：resize 只置脏、泵内统一 layout+draw，消除"每步重绘整窗"。
5. **S5**：表格 owner-data 虚拟列表，顺带清 ③ 闪烁。
6. 整窗按失效矩形裁剪（目标 ≤20 ms）、画布缓存位图（6 ms → <1 ms）。
7. `pp_app` 改读 `power_plan.xaml`（现在仍是手算 `_layout`）。
8. 待补容器控件：`SplitPane`、`SidePanel`、`OverlayCard`、`MiniMap`、`ToolBar`。
