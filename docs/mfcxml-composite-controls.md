# 复合控件层：现状、性能基线、改造顺序

> 结论先行：当前 60 多个控件全是「单个 HWND + 一段自绘」的薄壳，真正让界面好用、让 Python 不用手工拼装的东西
> ——复合控件（表格、侧板、分割容器、详情浮层、工具栏、小地图）——**一个都没有**。
> 界面好坏和性能问题都出在这层缺位上：Python 用 8 个 Edit + 3 个 Button 自己拼侧板，每次重排 40 个 HWND；
> 表格的重绘与数据都落在宿主里，跨 pybind 一趟一趟地调。所以下一步不是继续调单个控件，而是补齐复合控件层。

## 一、性能基线（`PYARX_D2D_RT=software`，39 节点 / 39 行 / 8 列，单次操作计时，取中位数）

| 操作 | 耗时 | 说明 |
|---|---|---|
| `pp_app._layout()` | **2.1 ms** | 40 个控件 `moveMany` 一次，本身不慢 |
| `fill_table()` 旧（清空 + 39 行 `addTableRow`） | **22.5 ms** | 每行 0.58 ms，跨 pybind 39 次 + `LVM_INSERTITEM` 39 次 |
| `fill_table()` 新（一次 `setTableRows`） | **15.6 ms**（最快 9.7） | 39 行整表一条消息，`WM_SETREDRAW(FALSE)` 包住插入，只失效一次 |
| `collect_grid()`（读回 39×7 格） | 0.2 ms | 只读，便宜 |
| `setTableSelectedRow` 单次 | 0.0 ms | 接口本身免费 |
| 逐行选中 39 行 | **129 ms** | 3.3 ms/行 —— 选中会触发 Python 回调 + 画布选中 + 详情，真正贵在这里 |
| 强制重绘 表格 `TBL` | **8.7 ms** | 39 行 × 8 列全部重画 |
| 强制重绘 画布 `CV` | **6.0 ms** | 39 卡片 + 8 条贝塞尔 |
| 强制重绘 侧板 `ED_PANEL` | **16.6 ms** | 一张白底矩形，却比表格还慢 → 它的 `WM_PAINT` 把整块侧板（含子控件区域）都画了 |
| 强制重绘 状态栏 `ST_PANEL` | 1.7 ms | |
| 强制重绘 小地图 `MINI` | 0.0 ms | |
| **强制重绘 整窗** | **82.1 ms** | 12 fps。任何一个 `InvalidateRect(top)` 级别的刷新就是这个数 |

读数：单控件都不算离谱，**贵在"一次操作要重画多少张面"以及"一次数据变化要跨多少次接口"**。
整窗 82 ms 意味着只要有一次全窗失效（重排、主题变化、窗口尺寸变化），就一定掉帧；
表格 22.5 ms 的填充意味着每次编辑提交、排序、粘贴、增删行都会闪一下。

## 二、控件分类（`G:\PyArx\MfcXamlCore\controls`，60 个头文件）

**基础控件（够用，不动）**：`XamlButton` `XamlTextBlock` `XamlTextBox` `XamlCheckBox` `XamlRadioButton`
`XamlComboBox` `XamlSlider` `XamlProgressBar` `XamlGroupBox` `XamlSpinBox` `XamlSwitch` `XamlRangeSlider`
`XamlDatePicker` `XamlHyperlink` `XamlToolTip` `XamlMenu` `XamlStaticLine` `XamlStaticBitmap` `XamlBusyIndicator`。

**薄壳（只是 `createNative(host, kind, id, ...)` 转手给宿主，自己没有任何状态与行为）**：
`XamlTableView` `XamlListCtrl` `XamlListView` `XamlSplitter` `XamlScrolledWindow` `XamlScrollBar`
`XamlTreeView` `XamlTabControl` `XamlToolBar` `XamlPopup` `XamlPageView` `XamlStackView` `XamlSwipeView`
`XamlGridView` `XamlPathView` `XamlTumbler` `XamlDial` `XamlMenuBar` `XamlPageIndicator` ——
**这些正是"复合控件"该长的地方，现在全是空壳。**

**自绘业务控件**：`XamlPowerCanvas`（配电画布，唯一有真实内部状态的）、`XamlCodeEditor`。

## 三、复合控件该长什么样（接口草案）

统一三条规矩（对应"布局由 XAML 定位、更新通过回调设定"）：

1. **几何全部来自 XAML**：控件只暴露 `Left/Top/Width/Height` 与内部子元素的比例/间距属性，
   自身绝不按窗口尺寸算坐标；窗口变化由 XAML 的 Grid 行列表驱动，控件收到新矩形即重排内部。
2. **数据与状态在控件里**：行、列、选中行、排序列、编辑态都由控件持有，Python 只给数据、拿事件。
3. **变化走回调**：控件发出 `SelectionChanged / CellEdited / SortRequested / ValueChanged`，
   Python 在回调里改模型并回写；禁止 Python 每次自己去 `setXxx` 十个接口再 `_layout()` 一遍。

优先级（按"界面好不好用 / 现在有多慢"排）：

| 序 | 复合控件 | 内部组成 | 关键属性（XAML） | 关键回调 | 解决的问题 |
|---|---|---|---|---|---|
| 1 | `TableView` | 列头 + 行区 + 单元格编辑框（内嵌，不浮在外面）+ 垂直滚动条 | `Columns/ColumnWidths/RowHeight/HeaderHeight/Editable/ShowGrid` | `SelectionChanged(row)` `CellEdited(row,col,text)` `SortRequested(col,asc)` `AddRowRequested` | 现在 8 个 Edit + 3 个 Button + 表格由 Python 摆；填充 22.5 ms、选中 3.3 ms/行 |
| 2 | `SplitPane` | 两个子面板 + 自带分割条（拖动状态机已在控件里） | `SplitRatio/MinFirst/MaxFirst/Orientation` | `SplitChanged(ratio)` | 分割条的算法已经收到控件里了，但容器还是 Python 拼的；分割后要手动重排 40 个控件 |
| 3 | `SidePanel` | 标题行 + 动作按钮组 + TableView + footer | `Title/Actions/Footer` | `ActionClicked(name)` | 侧板现在 20+ 个独立 id 由 Python 摆位 |
| 4 | `OverlayCard` | 圆角卡片 + 阴影 + 内容行 | `Anchor/Offset/Width/Padding` | 无（只是容器） | 详情浮层的 9 个 Label 现在按 `DT_X/DT_Y` 手工偏移 |
| 5 | `MiniMap` | 缩略位图 + 视口框 + 点击命中 | `Source/ViewportRatio` | `Navigate(x,y)` | 现在是小画布 + Python 算比例 |
| 6 | `ToolBar` | 按钮组 + 分隔 + 溢出折叠 | `Items/Overflow` | `ItemClicked(id)` | 现在 15 个按钮由 Python 逐个摆 |

## 四、重绘路径（性能的真正抓手）

现在的数据：整窗 82 ms / 表格 8.7 ms / 侧板 16.6 ms / 画布 6.0 ms。
三件事按顺序做，每件都要有前后对比数字：

1. **按失效矩形重画**：宿主皮肤层现在整块重画，`ED_PANEL` 一张白底要 16.6 ms 就是证据。
   改成只画 `ps.rcPaint` 覆盖到的行/卡片；表格按可见行区间画（`LVM_GETTOPINDEX` + 可见行数）。
2. **画布缓存位图**：视图变换（zoom/offset）与数据不变时，直接 BitBlt 上一帧的位图，不再跑 D2D 绘制；
   只有变换/数据/选中变化才重建。39 卡片重画 6 ms → 命中缓存应 <1 ms。
3. **表格批量灌数据**：`fill_table` 改成一次 `LVM_SETITEMCOUNT` + `WM_SETREDRAW(FALSE)` 包裹的批量插入，
   目标 39 行 < 5 ms（现在 22.5 ms）。

## 五、验收口径

- 复合控件：XAML 里写 `<XTableView Columns="..." />` 就能出完整可编辑表格，Python 只挂回调和给数据；
  `pp_app` 里不再出现 `E_F0+i`、`B_APPLYROW`、`L_EDIT_FOOT` 这类手工摆放。
- 性能：整窗重绘 ≤ 20 ms、表格重绘 ≤ 3 ms、`fill_table` 39 行 ≤ 5 ms、选中一行 ≤ 1 ms；
  上面三条都要在 `_perf_ctl.py` / `_perf_table.py` 里留成可重跑的基线脚本。

## 六、当前基线脚本

- `G:\PyArx\tests\mfc_xaml\_perf_table.py` —— 布局/填表/读回/选中
- `G:\PyArx\tests\mfc_xaml\_perf_table2.py` —— 表格重绘 / 整窗重绘 / 点击 / 滚动（本节数字来源）
- `G:\PyArx\tests\mfc_xaml\_perf_ctl.py` —— 逐控件重绘耗时

## 七、已落地的第一刀（表格灌数据）

`PyMfcHost::setTableRows(id, blob)`（行间 `\n`、格间 `\t`，`PyMfcWindow`/`PyMfcPanel` 均已绑定）替代
Python 侧“清空 + 逐行 `addTableRow`”。原来每插一行都在控件重绘开启状态下失效一次，39 行就是 39 次重绘；
现在 `WM_SETREDRAW(FALSE)` 包住全部插入、末尾只失效一次。

实测：`fill_table()` 22.5 ms → **15.6 ms**（最快 9.7 ms）；回归 `pp_test_all` 27/27、`pp_split_test` 13/13。

下一步仍是复合控件本身（`TableView` 内建列头/行区/单元格编辑/排序/滚动）与整窗按失效矩形裁剪——
15.6 ms 里剩下的部分是 39×8 条 `LVM_SETITEMTEXTW`，只有改成 owner-data 虚拟列表（数据在控件里、
按可见行画）才能再降一个量级。
