# MFCXaml 会话精华（2026-11-11）

> 交接口径：这份是"下次接着干必须先知道的事实"，不含过程叙述。数字都是本机软件 D2D 实测。

## 一、用户定下的架构规矩（最高优先级，别再改）

1. **MFCXaml 只提供控件库**（`Button/Label/TextBox/ListCtrl/ListBox/Panel/StaticLine/StaticBitmap` 等基础控件）。
   不在 C++ 里为业务界面写布局，也不在 Python 里算像素。
2. **复合控件用 XAML 组合基础控件**，放在 `G:\PyArx\plugins`，一个 `<Window Name="...">` 文件 = 一个复合控件，
   `x:Name` 是子控件名；几何全部来自 `M.xamlLayout(path, w, h)`。
3. **通用组合最终可以下沉成 C++ 控件类**（接口不变：XAML 标签名、属性、`on_*` 回调名都不改，
   只把创建与事件搬进 C++）。判断标准见 `plugins\README.md` 第三节（状态机/自绘/复用/热路径任一条）。
4. **所有回调一律回 Python**，两种写法等价可混用：
   - XAML 预先声明：`<Button Name="BtnApply" Text="应用" on_click="on_apply"/>`，加载时按名字在调用方作用域解析；
   - 运行期对象图赋值：`mybutton = pp_plugin.xamlload(path, app=..., x=, y=, w=, h=)`，然后 `mybutton.BtnApply.click = myclass.on_click`。
   事件名：`click / dblclick / change / grid`（`grid` 回调签名 `(action, row, col, text)`）。
5. 界面布局"大小交给 XAML"（用户四次强调）；`pp_app.py` 仍在手算坐标是**未完成项**。

## 二、本轮落地的东西

| 东西 | 位置 | 状态 |
|---|---|---|
| 复合控件加载器（XAML→对象图、`on_*` 解析、`xamlload`/`import_plugin`、`set_rows`/`move`） | `tests\mfc_xaml\pp_plugin.py` | 可用，5/5 PASS |
| 第一个复合控件：编辑侧板（Title/Table/3 Button，XAML Grid 定位） | `plugins\side_panel.xaml` | 可用 |
| 机制说明与"何时下沉 C++" | `plugins\README.md` | 已写 |
| 整表批量灌数据 `PyMfcHost::setTableRows(id, blob)`（行间 `\n`、格间 `\t`） | `PyMfcXaml\PyMfcHost.h/.cpp`，`PyMfcWindow`/`PyMfcPanel` 已转发并 `.def` 绑定 | 可用，回归全绿 |
| 复合控件清单 / 性能基线 / 验收口径 | `docs\mfcxaml-composite-controls.md` | 已写 |

## 三、性能基线（软件 D2D；39 节点 / 39 行 / 8 列；中位数）

| 操作 | 数值 |
|---|---|
| `pp_app._layout()`（40 个 `moveMany`） | 1.8 ms（旧 2.1） |
| `fill_table()` | **15.6 ms（最快 9.7）**，旧逐行插入 22.5 ms |
| `collect_grid()` | 0.2 ms |
| `setTableSelectedRow` 单次 | 0.0 ms |
| 逐行选中 39 行 | 129 ms（3.3 ms/行，含 Python 回调 + 画布选中 + 详情） |
| 强制重绘 表格 / 画布 / 侧板 / 状态栏 / 小地图 | 8.7 / 6.0 / **16.6** / 1.7 / 0.0 ms |
| **强制重绘 整窗** | **82.1 ms（≈12 fps）** |
| `show_editor + pump` | 465–483 ms |

结论：单控件不慢，**"一次操作重画多少张面"+"一次数据变化跨多少接口"** 才是瓶颈；侧板一张白底 16.6 ms 是宿主整块重画的证据。

## 四、`fill_table` 为什么变快（做法与边界）

`setTableRows` 把 39 次"插一行 → 失效重绘一次"合成 1 次：`WM_SETREDRAW(FALSE)` 包住全部
`LVM_INSERTITEMW` + `LVM_SETITEMTEXTW`，末尾只 `InvalidateRect` 一次。
剩下的 15.6 ms 主要是 39×8 条 `LVM_SETITEMTEXTW`，且数据在宿主与原生 `SysListView32` 各存一份；
**要再降一个量级只能改成 owner-data 虚拟列表（数据只在控件里、按可见行画）**，这就是 `TableView` 下沉 C++ 的正题。

## 四·补、画布光标（已修，规格已定）

规格：**画布内始终手形（悬浮与拖动都是），离开画布即复位成普通箭头**。
实现：`PyMfcProcs.cpp` 画布窗口过程 `WM_SETCURSOR` 一律 `IDC_HAND`；`WM_MOUSEMOVE` 里
`TrackMouseEvent(TME_LEAVE)` 登记，`WM_MOUSELEAVE` 里 `SetCursor(IDC_ARROW)` 复位。
坑：宿主不会替子窗口复位光标，只设不复位就会"手形挂满全窗"。
用例同步改成真鼠标驱动（`SetCursorPos` + `GetCursor`）：模拟 `WM_MOUSEMOVE` 不触发 `WM_SETCURSOR`，
读到的是残值——这就是旧用例 2/4 假失败的成因。

## 五、回归状态

- 全量自动测试 **118 项**（新增 `pp_click_test` 5 项）：`pp_test_all` 27、`pp_split_test` 13、`pp_cell_test` 15、
  `pp_event_test` 8、`pp_grid_test` 18、`pp_select_test` 7、`pp_real_test` 6、`pp_cursor_test` 5、
  `pp_click_test` 5、`pp_runtime_test` 9、`pp_plugin_test` 5。
- 已闭环两件事：星号列取整（`ResolveTracks` 余数补最后一列，640 宽现在正好 640）；
  点击 → XAML 声明回调（`pp_click_test` 5/5，同时验到属性赋值回调）。
- 新坑：XAML 声明名解析失败会抛 `NameError` 把属性赋值回调一起挡掉 → 改成"提示一次并跳过"，
  作用域改为取调用方模块 globals。
- 旧的 113 项全绿：`pp_test_all` 27、`pp_split_test` 13、`pp_cell_test` 15、`pp_event_test` 8、
  `pp_grid_test` 18、`pp_select_test` 7、`pp_real_test` 6、`pp_cursor_test` 5、`pp_runtime_test` 9、`pp_plugin_test` 5。
- 批次脚本：每跑一个 GUI 用例前杀 python 并 sleep 1.4 s（残留窗口会抢鼠标/焦点造成假失败）。
  连跑两个 GUI 测试之间必须杀 python 进程 + sleep 0.5~1.5 s。

## 六、未完成 / 待办（按优先级）

1. **点击 → 回调未闭环**：真实鼠标点击到 XAML 声明回调"被调到"只验证到"按钮 HWND 定位 + 回调已登记"；
   `_probe_plugin_cb.py` 在打印 `btn hwnd=40374448 rect=(22,356,205,34)` 后进程直接退出，没打印 `hits`。**先查这条**。
2. **XamlCore 星号列取整丢 1px**：640 宽 `12 + 3*star + 12` 算出 639（每列 floor）。应把余数分给最后一列星号列。
3. **`TableView` 下沉 C++**：owner-data + 内建列头/单元格编辑/排序/滚动，布局由 XAML 给，事件走回调。
4. **整窗按失效矩形裁剪**（目标 ≤20 ms）、**画布缓存位图**（6 ms → <1 ms）。
5. **`pp_app` 改读 `power_plan.xaml`**（现在仍是手算 `_layout`/`_layout_editor`，XAML 只有 `CV`/`SPLIT`/`Side` 三个区域）。
6. `SplitPane`（分割条状态机已在 `PyMfcHost::PyMfcDragBarProc` 内，容器还没做）、`SidePanel`、`OverlayCard`、`MiniMap`、`ToolBar`。

## 七、环境与操作要点（复现必需）

- Python `D:\Program Files\Python314\python.exe`（3.14.8）；MSVC `...\MSVC\14.51.36231`；
  构建：`$env:ComSpec=$env:COMSPEC='C:\Windows\SysWOW64\cmd.exe'`，
  `MSBuild.exe G:\PyArx\PyArx.sln /t:MfcXamlCore /t:PyMfcXaml /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false`，
  **编前必须关 python**（pyd 被占用 → LNK1104）。
- 测前 `$env:PYARX_D2D_RT='software'`，`cd G:\PyArx\tests\mfc_xaml`；脚本模板 `sys.path.insert(0, r'G:\PyArx\X64\Release')` + `os.add_dll_directory(...)`，收尾 `os._exit(0)`。
- `G:\PyArx` 不是 git 仓库，改动前备份到 `docs\_backup_*`；源码多带 BOM，改动用 `io.open(..., encoding='utf-8-sig')` + `newline=''`。
- 基线脚本：`_perf_table.py`（布局/填表/读回/选中）、`_perf_table2.py`（表格与整窗重绘）、`_perf_ctl.py`（逐控件重绘）。
  插件验证：`pp_plugin_test.py`、`_probe_plugin_cb.py`。
