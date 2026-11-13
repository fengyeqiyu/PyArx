# 配电规划图：MFCXaml 完整实现（对齐 power-plan.html）

源基准：`G:\配电规划设计\power-plan.html`（1842 行 / 68.9 KB）。
实现：Python + `PyMfcXaml.pyd`（自绘 `PowerCanvas` 控件 + 原生 MFC 控件），**不需要 CAD，独立 Python 直接运行**。

```
D:\Program Files\Python314\python.exe G:\PyArx\tests\mfc_xaml\pp_app.py
```

---

## 1. 功能对照（HTML 每一项 → MFCXaml 实现）

| # | HTML 原功能 | MFCXaml 实现 | 实测结果 |
|---|---|---|---|
| 1 | `#docTitle` 图名（contenteditable） | 工具栏 Edit，`onChange` 触发重新布图 | 改「改过命的配电图」→ 标题与图名同步，节点 4 ✓ |
| 2 | `#btnSample` 文档示例 | 13 节点内置样例（与 SAMPLE_CSV 同构） | 1480×436，13 卡 ✓ |
| 3 | `#btn3Level` 三级配电示例 | 39 节点（TR1→2 柜→每柜 3 路→每路 5 末端） | 6174×430，交叉 0，角色 root1/branch8/terminal30 ✓ |
| 4 | `#btnEdit` 节点编辑侧板 | 右侧浮出侧板（632×744、白底带边、贴窗口右边）：头部标题+计数+6 按钮、8 列只读表格、7 字段表单分两行；打开时主画布同时收窄到 360 宽（图自动重适配），右栏详情/小地图照常在 | 表格 13 行、改负载 9→12 生效并重绘；编辑态画布 22%，关闭后回到 62% 全图 ✓ |
| 5 | 导入 CSV | 文件对话框 → `powerplan.parseCsv`（认电压/长度列、`\t` 分隔、别名） | 4 节点 252×436 ✓ |
| 6 | 导入 XLSX | 标准库 `zipfile` + `xml.etree` 解 `sheet1.xml` / `sharedStrings.xml` → CSV → `parseCsv` | 读 5 行，转 4 节点 ✓ |
| 7 | 导入 JSON | `powerplan.parseJson`（含 `dir` 方向） | 回灌后尺寸/节点一致 ✓ |
| 8 | `#btnPaste` 粘贴 JSON | 读剪贴板 `CF_UNICODETEXT` → `parseJson` | 128 字符原文读回 ✓ |
| 9 | 导出 JSON | `view['json']` 落盘（2 位小数，含 layout/checks） | 4779 B ✓ |
| 10 | 导出 CSV | `view['csv']`，UTF-8 **带 BOM** + CRLF | 616 B ✓ |
| 11 | 导出 SVG | `view['svg']`（贝塞尔 path、电压角标、电缆标注、折叠 +N、图框标题栏） | 11978 B ✓ |
| 12 | 导出 PNG（2x） | `powerplan.renderPng(..., 2)` D2D 离屏 | 2960×872 / 97962 B ✓ |
| 13 | `#btnZoomOut/Reset/In` | −/100%/＋，围绕画布中心缩放，钳 5%–400% | 1.000→1.250→1.000→适应 ✓ |
| 14 | `#btnFit` 适应窗口 | 自己算 fit（pad 0.92，居中），下限 5% 保证 6 米宽图也能全貌 | 39 节点 15% 全图 ✓ |
| 15 | `#chkCable` 连线标注电缆 | `setPlanOptions(showCable)` → 绘制层直接关掉标注 | 关/开各一张截图 ✓ |
| 16 | `#dirSel` 布局（h/v） | 复选框「横向布局」→ `build(..., horizontal)` 重排 | 横向 1044×2238，交叉 0 ✓ |
| 17 | `#search` 搜索编号/名称 | 命中 → 自动展开折叠祖先 → `selectPlan` + 居中 + 填详情 | AT2-1-3 命中并居中 ✓ |
| 18 | 画布拖动/滚轮/单选/双击折叠 | 全部在 C++ 自绘画布里：滚轮围绕光标缩放、拖动阈值 3px、单击命中、双击回调 | zoom 0.148 下 hitPlan 正确 ✓ |
| 19 | 折叠 `computeHidden` + `+N` | Python 侧剔除折叠后代并补 `collapsedCount`，绘制层画 `+N` | 折 AT2-1 → 34 节点；折根 → 1 节点 ✓ |
| 20 | 小地图 `.mini`（180×126） | 312×196 自绘画布（`mini` 模式，不画电缆标注，叠红色视口框），**点击即跳** | 横向/纵向两图红框随主画布变化 ✓ |
| 21 | `#detail` 节点详情浮层 | 右侧 8 行常驻面板（编号/名称/上游/电缆/电压/负载/长度/下游数） | AT2-1-3 显示 220 / 5 kW / 9 m / 下游 0 ✓ |
| 22 | `#status` 状态栏 | `节点 N · 连线 M · 交叉 X · 重叠 Y · 380V a / 220V b[·中压 c][·高压 d] · zoom% | 提示` | 39 节点：380V 20 / 220V 18 / 中压 1 ✓ |
| 23 | `#banner` 错误横幅 | 错误统一写进状态栏（`布局失败：…` / `导入失败：…`） | — |
| 24 | 电压归一化 / 角标 / 电缆三段拼接 | C++ `normVoltage`/`voltClass`/`voltStyle`/`cableLine`，绘制层画角标与三行文字 | 10kV 绿、380 棕、220 蓝；`YJV22-4×95 · 10 m` ✓ |

---

## 2. 本轮修掉的 5 个真实缺陷（都是实测发现的，不是猜的）

1. **表格只显示表头，数据行全空**
   `addListCtrl` 建的是**原生 `SysListView32`**，而 `addTableRow` 只往自绘用的 `m_tblRows` 里存，从没往控件 `LVM_INSERTITEM`。
   → `addTableRow` / `clearTableRows` / `setTableSelectedRow` / `getTableSelectedRow` 全部补原生分支；顺带开了 `FULLROWSELECT | GRIDLINES | DOUBLEBUFFER` 与 13px 雅黑（否则默认 8pt，字比 HTML 小一号）。

2. **切到横向布局后画面还是纵向**
   `setPowerCanvasPlan` 里 `buildPlan(nodes, st.plan, style, err)` 用的是默认 `horizontal=false`，JSON 里的 `"dir":"h"` 被丢掉。
   → `parseJson` 增加 `bool* horizontal` 输出参数读顶层 `dir`，画布按 JSON 自己的方向重排（`[plan-cv] … dir=h` 已进日志）。

3. **小地图点击只能"近似居中"**
   小地图的 `k/ox/oy` 只在绘制时局部算，鼠标拿不到。
   → `PyMfcPlanCanvasMouse` 单击分支按同一公式反算模型坐标，直接写主画布的 `ox/oy`（主画布 1:1 平移到点击处，红框同步）。

4. **适应窗口后图看不全（左右被裁）**
   Python 侧 `fit_view` 沿用了滚轮下限 0.2，而 6174 宽的图在 1000 宽画布上需要 15%。
   → fit 下限放到 0.05（按钮/滚轮仍钳 5%–400%）。

5. **折叠 `+N` 看不见**
   屏幕绘制与 SVG 都把 `+N` 画在卡片右上角，那里正好是电压角标（带不透明底色）—— 39 节点图里完全被盖住。
   → 两者一起挪到卡片**左下角**（屏幕 `drawPowerPlanContent` 与 `toSvg`）。折叠根的截图里 `+ 3` 现在清晰可见。

附带修：`clipboard_text` 的 ctypes 默认 `restype=c_int` 把 64 位句柄截成 32 位，`GlobalLock` 指针一读就 access violation —— 显式声明 `restype/argtypes` 后读回 128 字符。

---

## 3. 实测数字（`pp_app_check.py` / `pp_app_check2.py`）

```
[初始] 三级示例 6174x430 节点 39 交叉 0
       状态栏: 节点 39 · 连线 38 · 交叉 0 · 重叠 0 · 380V 20 / 220V 18 · 中压 1 · zoom 15%
[横向] 1044x2238 交叉 0
[搜索] 选中 = 'AT2-1-3' 视口 = (0.148, -31, 315)
[折叠] AT2-1 后节点数 = 34  下游被隐藏 = True
[展开] 节点数 = 39
[文档示例] 节点 13 尺寸 1480x436
[编辑面板] 表格行数 = 13
[编辑一行] AT1-1 负载 9 -> 12.0，重绘后仍 13 节点
[导出] json 4779 B / csv 616 B / svg 11978 B / png 2960x872 97962 B
[CSV]  节点 4  尺寸 252x436
[XLSX] 读到 5 行 → 节点 4  尺寸 252x436
[JSON] 节点 4  尺寸 252x436
[改名] 标题 '改过名的配电图'  节点 4
[缩放] 1.000 -> 放大 1.250 -> 100% -> 适应 1.000
[剪贴板] 128 字符读回
[折叠根] 节点数 1（应 1）→ [展开根] 4
```

截图（`G:\PyArx\docs\`）：
`_pp_app_main.png` 全图/15%；`_pp_app_horiz.png` 横向列流；`_pp_app_editor.png` 右侧浮出侧板（白底 + 13 行表格 + 两行表单，画布同时收窄）；`_pp_app_after_editor.png` 关闭侧板后画布恢复 62%；`_pp_app_csv.png` CSV 导入后的四节点图；`_pp_app_collapsed.png` 折叠 +N；`_pp_app_search.png` 搜索定位；`_pp_app_mini.png` 小地图；`_pp_probe_table.png` 表格最小验证。

---

## 4. 已知边界（未做就是没做）

- **文件对话框无法自动化**：`GetOpenFileNameW` / `GetSaveFileNameW` 是模态窗，脚本里全部走"对话框之后的逻辑"验证；弹窗本身需要人手点一次确认。结构体/Flags 按 x64 布局手写（`OFN_EXPLORER|FILEMUSTEXIST|PATHMUSTEXIST`，保存加 `OVERWRITEPROMPT`），**未做人工点击实测**。
- XLSX 只读**第一个 sheet**；不处理合并单元格、公式（读到公式缓存值）、日期序列号；也就不支持多 sheet 选择。
- 侧板版式与 HTML 的 640 宽侧板一致（贴右、白底、左边框），但**表格是只读的**：HTML 是每格一个 `input` 的行内编辑，原生 `SysListView32` 没有"单元格内嵌输入框"这回事，所以改成「点行 → 读取选中行 → 改表单 → 写入并重绘」。真要做行内 input 得给每格叠一个 Edit（13 行 × 7 列 = 91 个控件）并自己管滚动与 Tab 导航，本次没做。
- 搜索命中只有红色高亮，没有 HTML 的 `.pulse` 呼吸动画。
- SVG 侧（`nanosvg`）已知边界不变：渐变近似、`<g transform>` 只认 translate、不支持 `<image>/clipPath/mask/<use>`。
- 折叠状态点画在卡片右上角为 `+N` 文本，没有 HTML 里"折叠节点虚线轮廓"。
- 重叠数（状态栏 `重叠 Y`）由 Python 按卡片矩形两两相交实时算，不是布局器内部统计。

---

## 5. 相关源码位置

| 改动 | 文件 |
|---|---|
| 应用主体（工具栏/画布/小地图/详情/编辑/导入导出/搜索/折叠） | `G:\PyArx\tests\mfc_xaml\pp_app.py` |
| 验收脚本 | `pp_app_check.py`、`pp_app_check2.py`、`pp_probe_table.py`、`pp_probe_cv.py` |
| 原生表格行 / 字体 / 小地图精确点击 / JSON 方向 | `G:\PyArx\PyMfcXaml\PyMfcXaml.cpp` |
| 侧板白底 `addWhiteRect`（新接口，默认退化） | `XamlMfcCore\controls\XamlMfcHost.h` + `PyMfcXaml.cpp`（`SS_WHITERECT \| WS_BORDER`，`PyMfcWindow/PyMfcPanel` 各一层 wrapper） |
| `parseJson` 读 `dir` | `G:\PyArx\MfcXamlCore\XamlPowerPlan.h` / `.cpp` |
| 绘制（屏幕与离屏共用） | `PyMfcXaml.cpp` 全局 `drawPowerPlanContent` |

## 6. 闪烁 / 拖影：根因与修复（实测定位，非推测）

用户原话「界面闪烁厉害，找出原因」「拖影太严重了」。逐条查下来是四个独立原因叠在一起：

| # | 位置 | 原因 | 修法 |
|---|---|---|---|
| 1 | `moveMany` | 每个控件 `InvalidateRect(c, nullptr, TRUE)` 先把控件擦成白底再重画，一帧里白→内容 → 肉眼"闪一下" | erase 参数改 `FALSE`（控件自己首行 `fillRect` 铺底，本来就不需要预先擦） |
| 2 | `setPowerCanvasView` | 同上，缩放/平移每一帧都擦白一次 | 同样 `TRUE` → `FALSE` |
| 3 | 主窗 `CreateWindowExW` | 没有 `WS_EX_COMPOSITED`，子控件各自往屏幕画，重排时能看到中间态 | 加 `WS_EX_COMPOSITED`（整窗离屏合成，重排只在最后合成一次） |
| 4 | `moveMany` 改控件位置 | 控件挪走后，旧位置属于**父容器**；父容器没标脏，主窗又带 `WS_CLIPCHILDREN` 把这块裁掉 → 旧内容原地不动 = 拖影（用户截图里表格区出现两行重复） | 对 `::GetParent(c) != m_hWnd` 的父容器补 `InvalidateRect(p, nullptr, FALSE)` |

修复后整窗截图 `docs/_pp_live.png`：表格 39 行无重复残影、右侧无灰带、画布与状态栏对齐。

## 7. 控件级导出 PNG（基类能力）

`PyMfcHost::saveControlPng(id, path)` —— 任意控件（含 D2D 自绘画布、原生 ListView）都能按当前外观导成 PNG：

```python
w.saveControlPng(CV,  r'G:\PyArx\docs\_ctl_canvas.png')   # 33638 B
w.saveControlPng(TBL, r'G:\PyArx\docs\_ctl_table.png')    # 60081 B
```

做法：`PrintWindow(c, memDC, PW_RENDERFULLCONTENT=2)` 画进 32bpp 自顶向下 DIB → `GdiFlush` → `wicWritePng`。
自绘控件之所以能抓到而不是黑图，是因为 `PyMfcOwnerDrawProc` 新增了 `WM_PRINTCLIENT` 分支：拿到外部 HDC 后按
和 `WM_DRAWITEM` 完全同一条路径重画一遍。Python 侧 `PyMfcWindow` / `PyMfcPanel` 都转发了这个方法。

## 8. 事件驱动验证（不需要真人操作）

`tests/mfc_xaml/pp_events.py` 把模拟输入封成函数，全部走 `user32.SendMessageW` 直接投递到控件过程：

```python
cv  = E.find_control(app.win.hwnd, pp_app.CV)   # 递归找句柄（GetDlgItem 取不到自绘控件）
E.click(cv, 140, 389)                           # 单击
E.double_click(tbl, 60, 60)                     # 双击
E.drag(cv, 400, 300, 560, 400)                  # 按住拖动
E.wheel(cv, 120, 300, 300)                      # 滚轮（lParam 用屏幕坐标）
E.key(tbl, 0x71)                                # F2 打开行编辑条（VK_F2=0x71，不是 0x72）
```

`pp_event_test.py` 8 项全 PASS：找到画布/表格句柄、单击选中卡片、滚轮 `0.2000→0.2240` 并反向缩回、
拖动视口 `(36,334)→(196,434)`、F2 展开行编辑条、取消收起。

## 9. 「选不中画布里的节点与线」的真因

不是命中测试的问题，是**单击被丢掉了**：

```
WM_LBUTTONUP:  if (GetCapture()==hwnd) ReleaseCapture();   // ← 同步发出 WM_CAPTURECHANGED
               const bool wasDrag = st.drag;              // ← 已经被那个分支清成 false
```

`ReleaseCapture` 会**同步**回调本窗口的 `WM_CAPTURECHANGED`，而那个分支里 `st.drag = false`；
所以 `wasDrag` 永远读到 false，单击分支根本不执行 → `planHit` 从来没被调用过 → 选不中。
（`hitPlan()` 直接调 API 是好的，这个反差就是定位线索。）

修法：**先把状态取下来再释放捕获**（`wasDrag = st.drag; st.drag = false; if (GetCapture()) ReleaseCapture();`）。
同时补了两件：`planHit` 在盒命中失败后做贝塞尔折线最近距离（24 段、容差 `8.0f/zoom`，命中返回 `l.to`）；
Python 侧补上 `w.onClick(CV, self.on_canvas_click)` —— 之前只绑了 `onDoubleClick`，`on_canvas_click` 存在但从没接上。

`pp_select_test.py` 7/7 PASS：单击卡片 → `getPlanSelection` 得 `AT1-1` + 详情浮层打开、点空白清空、
点连线中点 (242,378) → 选中下游 `AT1-1`、两个控件级 PNG 落盘。

## 10. 回归基线

`pp_grid_test.py` **18/18**；`pp_test_all.py` **27/27**；`pp_select_test.py` **7/7**；`pp_event_test.py` **8/8**。

## 11. 「真实鼠标选不中 / 滚轮不缩放」的真因：WM_NCHITTEST 回了 HTTRANSPARENT

现象：SendMessage 模拟点击 7/7 全过，但真人用鼠标点卡片没反应、滚轮也不缩放。

排查顺序（每一步都是实测，不是猜）：

1. `WindowFromPoint(卡片中心)` 返回的不是画布句柄，而是一个 id=0 的容器；
2. 枚举该点下的子窗口，覆盖该点的只有画布自己（rect、`IsWindowVisible`、Z 序都正常）；
3. `GetWindowRgn` 返回 0（没有裁剪区域）；
4. 直接给画布发 `WM_NCHITTEST` → **返回 -1**。

`-1` 是 `HTTRANSPARENT`：**"我这个点在命中测试上透明，把点击交给下面的窗口"**。
画布是 `SS_OWNERDRAW` 的 Static，Static 的类过程默认就这么回 —— 绘制被我们接管了，
但命中声明没跟着改。于是系统把真实鼠标全部发给父窗：卡片点不中、滚轮也滚不到。
而 `SendMessage` 是直接投递到窗口过程、**绕过系统命中链**，所以模拟点击一直是好的。
这个"模拟通、真实不通"的反差就是根因线索。

修法（`PyMfcOwnerDrawProc` 最前面，和皮肤过程里已有的约定对齐）：

```cpp
if (msg == WM_NCHITTEST)
    return HTCLIENT;
```

另外记录一个测试方法上的坑：真实输入（`SendInput`）投递的消息同样要靠宿主
消息循环分发，脚本里 `SendInput` 之后不 `M.pump()` 就立刻读状态，读到的是旧值 ——
这一点曾让我误判成"仍然点不中"。现在 `pp_real_test.py` 每次真实输入后都 pump。

`pp_real_test.py`（真实鼠标，不是 SendMessage）**6/6 PASS**：
系统命中落在画布、真点击选中 `AT1-1` 且详情浮层打开（探针 `up=(140,389) wasDrag=1 hit=AT1-1`）、
真滚轮 `0.2000→0.2240` 并缩回、真左键拖动平移。
