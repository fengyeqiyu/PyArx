# MFCXaml 渲染重构 S4 / S5：施工规格（布局与重排分离 · 表格 owner-data + ③⑤）

> 口径：本轮**只读**，未编译、未启动 GUI、未杀进程、未改任何源文件/配置；`mfcxaml-s2-migration-map.md`、`mfcxaml-s3-spec.md` 已冻结，本文不回改。
> 每条结论带 `文件:行号`；算术给算式；证据不足标「待实测」；**不给实现代码片段**，只给规格与判定判据。
> 源码基准：`G:\PyArx\PyMfcXaml\*`、`G:\PyArx\MfcXamlCore\*`、`G:\PyArx\tests\mfc_xaml\pp_app.py`；性能证据 `G:\PyArx\docs\_perf_newtable.txt`。
> 注意：源码行号以当前磁盘文件为准；前序文档引用的部分 C++ 行号随迭代漂移，本文已用当前复核行号。

---

# 第一部分 S4：布局与重排分离

## 1. 现状取证：尺寸变化 → 立即 layout → 立即 Invalidate/paint 完整链

### 1.1 调用链（自顶向下，逐条行号）

| 步 | 环节 | 文件:行号 | 行为 |
|---:|---|---|---|
| 1 | 顶层窗口 `WM_SIZE` | `PyMfcWrap.cpp:5-7`（消息映射）、`PyMfcTopWnd::OnSize` `PyMfcWrap.cpp:9-16` | `CWnd::OnSize` L11；`MoveWindow(0,0,cx,cy,TRUE)` 给宿主 `L13`；调 Python resize 回调 `L14-15` |
| 2 | 宿主窗口 `WM_SIZE` | `PyMfcProcs.cpp:25`（`ON_WM_SIZE`）、`PyMfcHost::OnSize` `PyMfcProcs.cpp:1132-1138` | 先 `CWnd::OnSize` L1134；表面路径 `m_surface->Resize(cx,cy)` L1136-1137；**宿主 C++ 侧不做布局、不直接 `InvalidateRect`** |
| 3 | 回调桥接 | `PyMfcWindow::setResizeCallback` `PyMfcWrap.cpp:200-201`；lambda 安装 `PyMfcWrap.cpp:71,179`；`notifyResize` `PyMfcWrap.cpp:211-219`（GIL 取回调、`m_resize(w,h)` L216）；Python 注册 `w.onResize(self.onResize)` `pp_app.py:464` | 顶层 `resize(cx,cy)` → `notifyResize` → Python `onResize` |
| 4 | Python 重排入口 | `PyMfcHost::onResize` `pp_app.py:646-648` → `self.layout()` L648 | 同步触发整窗网格重排 |
| 5 | Python `layout()` | `pp_app.py:497-535` | `clientSize` L503；`sideWidth/CV_MIN_W` 算 `cvW/cvH` L504-507；组装 `items`（画布/状态条/小地图/侧板/详情浮层）L509-533；`w.moveMany(items)` L535 |
| 6 | 侧板内部布局 | `layoutEditor` `pp_app.py:537-545+`；编辑器开关 `showEditor` `pp_app.py:1630-1635`、`hideEditor` `pp_app.py:1646-1651` 均结尾 `self.layout()` | 侧板宽/表格/按钮位置同步算出 |
| 7 | C++ 批量移动 | `PyMfcHost::moveMany` `PyMfcHost.cpp:3064-3121` | `BeginDeferWindowPos` L3069；逐项 `updatePaintNodeRect` L3090；`DeferWindowPos` 带 `SWP_NOREDRAW` L3091-3101；`EndDeferWindowPos` L3107 |
| 8 | 标脏 | `moveMany` 内：动过的子窗 `InvalidateRect(child, nullptr, FALSE)` L3112-3114；非主窗父窗补标 L3117-3118；主窗只标「动过的并集矩形」L3115-3116,3120-3121（不用 `RDW_ALLCHILDREN`，注释 L3108-3111） | 重排后立即把更新区交给消息队列 |
| 9 | 出帧 | 子窗各自 `WM_PAINT`（owner-draw `PyMfcHost.cpp:427-456`；皮肤链 `PyMfcProcs.cpp:1510`；表格 `Table.cpp:633`）；宿主 `OnPaint` `PyMfcProcs.cpp:1071-1119`；表面帧 `BeginFrame/EndFrame` `PyMfcProcs.cpp:1081-1101` | 标脏后由泵派发绘制 |
| 10 | 装载后自动 fit（非 resize，但同属「数据到达立即算视图 + 立即失效」） | `addPowerCanvas` 内建窗后 `fitPlanCanvas` `PyMfcHost.cpp:3652`；`setPowerCanvasPlan` `fitPlanCanvas` L3677 + `InvalidateRect(...,TRUE)` L3683；`setPowerCanvasScene` `fitPlanCanvas` L3700 + `InvalidateRect(...,TRUE)` L3706；SVG 分支 L3733/L3738 | 装场景即算 `zoom/ox/oy` 并标脏；Python 随后 `rebuild(fit=True)` 再覆写（`pp_app.py:1136-1143`，见 S3 规格 §3） |

### 1.2 现状「同步性」要点

- `moveMany` 是同步 Win32 调用：`DeferWindowPos/EndDeferWindowPos` 当场完成窗口矩形变更（`PyMfcHost.cpp:3069,3101,3107`），只是 `NOREDRAW` 把绘制延后到消息队列。
- 现有合并节流只在**分割条拖动**路径：`onSplitDrag` 33 ms 时间窗 + 6px 位移阈值，位移小且在窗内则直接 `return` 不重排（`pp_app.py:627-638`）；窗口 `onResize` 路径**没有**合并节流，来一次 `WM_SIZE` 排一次。
- 已存在节点脏标记字段：`XamlPaintNode.rc/visible/dirty`（`PyMfcHost.h:407-417`），`updatePaintNodeRect` 写矩形并置 `dirty=true`（`PyMfcHost.cpp:3234-3241`）；这是 S4 脏标记可直接复用的落点。

## 2. 现状：泵、定时时钟、空闲处理

### 2.1 宿主消息泵（Python 显式驱动）

- 绑定 `pump(maxMs)`：`PyMfcBind.cpp:603-637`。
  - `PeekMessageW(PM_REMOVE)` 循环派发 `TranslateMessage/DispatchMessage` L609-615；`WM_QUIT` 返回 `false` L611-612；`dispatched` 记录本次是否真派过消息 L606,615。
  - `maxMs>0`：**派发过消息且队列排空即提前返回** `L621-622`；一次没派发则 `MsgWaitForMultipleObjects(QS_ALLINPUT, maxMs-elapsed)` 等消息，超时返回 `true` L625-630。
  - `maxMs=0`：`MsgWaitForMultipleObjects(...,INFINITE,...)` 阻塞到 `WM_QUIT` L632。
- 另有截图侧固定循环泵：`PyMfcWrap.cpp:99-111`，`RedrawWindow(RDW_INVALIDATE|ALLCHILDREN|UPDATENOW)` 后 `60 × (Peek+Sleep(10))`，是独立于 `pump()` 的硬编码地板。

### 2.2 定时器（控件级 `WM_TIMER`，无宿主空闲定时器）

| 时钟 | 安装/处理 | 文件:行号 |
|---|---|---|
| 动画时钟 `kAnimTimerId` | 处理 `WM_TIMER` L1496 | `PyMfcProcs.cpp:1456,1496` |
| 光标闪烁 `kCaretTimerId` | `SetTimer` L1540 / `KillTimer` L1544；`WM_TIMER` 刷 `InvalidateRect` L1547-1552 | `PyMfcProcs.cpp:1540,1544,1547,1552` |
| Busy `kSkinBoxBusy` | `SetTimer` `PyMfcHost.cpp:1331-1333`；处理 `PyMfcProcs.cpp:1577-1581` | 同上 |
| 分页/翻页/工具提示 | `WM_TIMER` 处理 L1584-1600，到点 `KillTimer` | `PyMfcProcs.cpp:1584,1588,1591,1595,1598-1600` |

- 未发现 MFC `OnIdle` 钩子或宿主级重入空闲定时器；「空闲即跑 layout/draw」目前不存在，绘制由 `InvalidateRect` + 泵派发。

### 2.3 「pump 当等待用」现状计数（本轮实测匹配）

- 匹配口径：`tests\mfc_xaml\*.py` 内 `M.pump(` / 封装 `pump(` 文本匹配。**当前共 440 处**，分布在约 87 个 `.py` 文件；头部集中在 `pp_test_all.py` 26、`_s1_probe.py` 21、`pp_grid_test.py` 17、`pp_cell_test.py` 13、`_s1v_scene.py`/`_s2a_scene.py` 各 12、`pp_app_check.py` 12、`pp_events.py` 11（完整清单为本轮 PowerShell 匹配结果，未落盘）。
- 历史口径「371 处」「256Hz 空转」来自前序踩雷记录（hint 背景）；当前 440 与 371 的差集、以及哪些是真等待/哪些是出帧等待，**待实测分类**，不在本轮猜。
- 等待语义回归探针：`_probe_pump_wait.py:2,13-17`——空队列下 `pump(ms)` 必须按预算等待，不能 0~2ms 立即返回；代表预算 20/80/120/200（L12 注释）。
- 典型「先动后 pump 再断言/截图」用法：`_probe_split3.py:26,33`（拖动段 `pump(30)` 计时）、`_perf_detail_e2e.py:39,50,58,78`、`_perf_fix3.py:24,37`、`pp_real_test.py:89,99,105,107`、`_shot_after.py:10-14`。

## 3. S4 规格：resize 只置脏 + 泵内统一 layout+draw

### 3.1 最小改动面（文件清单）

| 文件 | 角色 | 改动边界（规格，不含代码） |
|---|---|---|
| `PyMfcWrap.cpp` | 顶层 `OnSize`/回调桥 | `OnSize`（L9-16）不再同步驱动 Python 整窗 layout；改为记录待处理尺寸/置脏标记。`notifyResize`（L211-219）语义保留，供节点布局调度器读取 |
| `PyMfcProcs.cpp` | 宿主 `OnSize` L1132-1138 | 表面 `Resize` 保留；尺寸变化只更新表面尺寸记录，不触发控件级同步重排 |
| `PyMfcHost.cpp` | `moveMany` L3064-3121、`updatePaintNodeRect` L3234-3241、`XamlPaintNode` L407-417 | 复用现有 `rc/visible/dirty` 字段；新增「待 layout」脏标记归属（窗口级 / 节点级二选一，见 3.2）；`moveMany` 仍同步改矩形的 Win32 部分保留给保留 HWND 覆盖层；节点矩形只进脏队列 |
| `pp_app.py` | `onResize` L646-648、`layout` L497-535、`onSplitDrag` L616-648、`viewArea` L1175-1178 | layout 计算变为可被调度的纯函数；resize/拖动只置「待 layout」；layout 结果在泵内同一帧提交 |
| `PyMfcBind.cpp` | `pump` L603-637 | 泵派发完消息、队列排空前执行一次「待 layout → 置脏 → 出帧」；不得引入 `Sleep` 地板或 INFINITE 空转 |

### 3.2 脏标记与触发条件规格

1. **脏标记落点**：窗口级 `pendingLayout` 布尔 + 脏矩形（复用 `XamlPaintNode.rc/dirty`，`PyMfcHost.h:410,414`；`PyMfcHost.cpp:3238-3240`）。窗口尺寸变 → 置 `pendingLayout=true` 并记新客户区；节点矩形变 → 对应 `dirty=true`，不另起同步重排。
2. **layout 触发条件**：泵内一轮 `PeekMessage` 排空后，若 `pendingLayout` 为真，执行一次 layout（Python 网格计算 → 节点矩形表 / 保留 HWND `moveMany`）；一帧内多次尺寸/拖动事件合并为一次 layout（沿用 `onSplitDrag` 33ms/6px 节流思路，`pp_app.py:631-638`，但把合并点从 Python 回调前移到调度层）。
3. **与节点 dirty/rect 衔接**：layout 结果写节点 `rc/visible` 并清 `dirty`（对齐 `updatePaintNodeRect` 现状 `PyMfcHost.cpp:3238-3240`）；出帧只画 `dirty=true` 节点 + 脏矩形并集；`moveMany` 的 `NOREDRAW` 语义保留（`PyMfcHost.cpp:3091`），绘制延后到本帧出帧。
4. **保留 HWND 覆盖层**：Edit/ComboBox/按钮等仍由 Win32 移动；其 `InvalidateRect` 维持现状按并集标脏（`PyMfcHost.cpp:3112-3118,3120-3121`），不并入节点脏队列。

### 3.3 性能与空转判据（可验证）

| 判据 | 采集方法 | 目标 |
|---|---|---|
| 每帧 layout 次数 | 在调度层计数：一次泵排空周期内 layout 执行次数 | 尺寸/拖动连续事件下 **1 次/帧**；无事件 0 次 |
| 每帧 Present 次数 | 表面 `Present` 调用计数（`XamlSurface.cpp:101-102`） | 一次出帧 **1 次 Present**；空泵不产生 Present |
| resize 后首帧 ms | 探针计时：`WM_SIZE` 到首帧变白；对照 `_probe_split3.py:26,33` 现有口径 | 不劣于现状；拖分割条 **≤60 ms/步**（现状约 34.4 ms，`_perf_newtable.txt:12-13`，不得回退） |
| 空转 | 空闲状态采样 `pump()` 频次/CPU；对照 `_probe_pump_wait.py` 等待语义 | 无事件时不跑 layout、不出帧；`pump(ms)` 空队列按预算等待，不提前 0~2ms 返回，也不空转 |
| 布局合并正确性 | 拖动松手（`phase=='up'`）补最后一帧（现状 `pp_app.py:625,644`） | 松手后节点/HWND 矩形与最终 `layout()` 一致，无停在旧位置 |

### 3.4 风险：依赖「resize 后立即拿到新几何」的用例与兼容方案

**同步语义被破坏的点**：现状 `onResize→layout→moveMany` 同步完成 Win32 矩形变更（`PyMfcHost.cpp:3101,3107`），Python/探针在同一段代码内（或 `pump` 之前）读控件矩形/位置时拿到的是新值；改成泵内延迟 layout 后，未派发泵之前读到的是旧矩形。

| 风险场景 | 现状证据 | 兼容规格 |
|---|---|---|
| 探针/脚本 resize 后立即读窗口/控件矩形再断言 | `_probe_rect.py:4`（`showEditor`+多次 `pump` 后取矩形）、`_diag_cover.py:46-51`、`_diag_rgn.py`、`pp_real_test.py:89+` | 这些用例均在几何读取前 `pump` 出帧；规格要求：**resize 回调同步先跑一次 layout 再置脏**（同步几何语义保留，只把绘制延迟到泵），即「同步算矩形、延迟出帧」；若调度层发现某路径未先同步算过，读取接口回退到同步 layout 一次 |
| 分割条拖动中读画布宽/侧板宽 | `onSplitDrag` `pp_app.py:616-644`；`layoutEditor` L537-545 | 拖动参数（`_split_tip/_ed_w`）是 Python 状态，layout 计算仍可同步取到；仅出帧延迟。测试用 `pp_split_test.py:45-80`、`_probe_split3.py` 回归 |
| 编辑器开关切换后表格/按钮布局 | `showEditor` L1630-1635、`hideEditor` L1646-1651 结尾 `layout()`；`pp_grid_test.py`、`pp_cell_test.py` 多次开关后操作行 | 同兼容方案：切换回调内同步 layout 一次，置脏延迟出帧；表格列宽/行高读取接口在未出帧前返回待提交矩形 |
| 详情浮层定位依赖画布新宽 | `layout` 详情段 `pp_app.py:527-533`；`positionDetail` `pp_app.py:1363-1365` | 浮层矩形随画布节点矩形在同一次 layout 内算出；显隐沿用 `_detail_visible/_detail_rc`（`pp_app.py:310,528-533`） |
| `pump()` 被当短等待 | §2.3 共 440 处匹配 | 调度器在 `pump` 排空后只补一次待处理 layout，不新增等待；`_probe_pump_wait.py` 等待语义必须保持绿 |

> 待实测：440 处中哪些在 `pump` 之前读几何、哪些只在 `pump` 之后断言；分类完成前，默认采用「同步 layout 算矩形 + 延迟出帧」的保守兼容口径，不做纯延迟重排。

---

# 第二部分 S5：表格 owner-data + ③滚动闪烁 + ⑤滚动条

## 4. 现状取证

### 4.1 每帧 GDI 对象创建/释放点（`TableControl::OnPaint` `Table.cpp:717-822`）

| 资源 | 创建 | 释放 | 行号 |
|---|---|---|---|
| 设备句柄 | `BeginPaint` L720 | `EndPaint` L823 | L720,823 |
| 兼容 DC | `CreateCompatibleDC` L724 | `DeleteDC` L822 | L724,822 |
| 兼容位图 | `CreateCompatibleBitmap` L725 | `SelectObject` 还原 L820、`DeleteObject` L821 | L725,820,821 |
| 字体 | `CreateFontW` L732-735（字号 `MulDiv(m_pt,LOGPIXELSY,72)` L731） | `SelectObject` 还原 L816、`DeleteObject` L817 | L731-735,816-817 |
| 表头刷 | `CreateSolidBrush(m_hdrBg)` L740 | `DeleteObject` L742 | L740-742 |
| 选中行刷（逐行） | `CreateSolidBrush(m_selBg)` L774 | `DeleteObject` L776 | L774-776 |
| 行分隔线刷（**每行一次**） | `CreateSolidBrush(m_line)` L792 | `DeleteObject` L795 | L792-795 |
| 滚动条 track 刷 | `CreateSolidBrush` L801 | `DeleteObject` L803 | L801-803 |
| 滚动条 thumb 刷 | `CreateSolidBrush` L810 | `DeleteObject` L813 | L810-813 |
| 离屏合成 | 内存 DC `FillRect` L727；行/格 `DrawTextW` L753,760,789；`BitBlt SRCCOPY` L819 | — | L727,753,760,789,819 |

- 每行一次线刷（L792）+ 选中行一次刷（L774）是随行数放大的 churn；字体/DC/Bmp 是每帧固定 churn。

### 4.2 行数据来源、索引映射、行高与上限

| 项 | 现状 | 文件:行号 |
|---|---|---|
| 原始行存储 | `m_rows: vector<vector<wstring>>`；`Rows()=m_rows.size()` | `Table.h:128,83`；追加 `AppendRow` `Table.cpp:359-360` |
| 列宽/标题/类型 | `m_colW` `Table.h:129`；`m_colTitle` `130`；`m_colType` `134`；`Cols()=m_colW.size()` `Table.h:84`；`SetColumns` `Table.cpp:509-515` | 同上 |
| 筛选视图 | `m_filter` `Table.h:135`；`RebuildView` 逐列子串匹配（小写），保留数据行号进 `m_view` `Table.cpp:363-384` | 筛选 `SetFilter` `403-410`；清空 `418-423` |
| 排序 | `SortBy` `stable_sort(m_view)`，保持数据行号映射 `Table.cpp:425-430` | 与 `m_rows` 解耦 |
| 可视首行 | `m_scroll`（数据行号在视图内的位置）`Table.h:133`；`ScrollBy` 钳 `[0,MaxScroll()]` `Table.cpp:582-589` | — |
| 定高 | 表头 `m_headerH=24`、行高 `m_rowH=21`（固定，非自适应）`Table.h:132` | — |
| 行上限 | 控件内**无行数上限**（vector 增长）；`Rows()` 只受 `m_rows.size()` 约束 `Table.h:83`。Python 编辑器行编辑覆盖层上限 `GRID_MAX_ROWS=44`（`pp_app.py:64`）属于另一套行编辑浮层，**不是表格数据上限**，勿混用 | `Table.h:83`；`pp_app.py:64` |

### 4.3 现有命中/几何算法

| 函数 | 算法 | 文件:行号 |
|---|---|---|
| `VisibleCount` | 可视高/行高，`viewH>0 ? viewH/m_rowH : 1` | `Table.cpp:532-537` |
| `MaxScroll` | `all>vis ? all-vis : 0` | `Table.cpp:539-544` |
| `RowRect` | 视图位置 `ViewPos - m_scroll`；`top=m_headerH+pos*m_rowH`；越界返回空 `RECT` | `Table.cpp:546-556` |
| `Cell` | 列宽累加；`top=m_headerH+(ViewPos-m_scroll)*m_rowH` | `Table.cpp:558-569` |
| `Hit` | `pos=m_scroll+(y-headerH)/m_rowH`；`pos∈[0,ViewRows)`；行=`m_view[pos]`；列=`ColAtX(x)` | `Table.cpp:571-580`；`ColAtX` `392-401` |
| `EnsureVisible` | 行早于首行前移、晚于末行后移，再钳 `MaxScroll` | `Table.cpp:591-601` |
| 绘制行区间 | `ViewRows()` 循环，`rr.top>=rc.bottom` 提前 `break`（`Table.cpp:766-771`）；**但循环从 pos=0 起，非从可视首行起算** | L766-771 |

## 5. S5 owner-data 规格（只灌可见行）

### 5.1 前置改造项（规格）

1. **视口行区间计算**：由 `m_scroll` + `VisibleCount()` 给出首/尾视图位置 `[m_scroll, min(m_scroll+vis, ViewRows())]`；绘制与灌数据只遍历该区间，替换现有从 pos=0 起的循环（`Table.cpp:766-771`）。`RowRect/Hit` 现有公式已基于 `m_scroll`，可直接复用（`Table.cpp:546-580`）。
2. **行高定高**：现 `m_rowH=21/m_headerH=24` 固定（`Table.h:132`），区间换算无需行高累加表；若未来改行高，需先引入行高数组再谈区间。
3. **数据源接口**：保持 `m_rows[m_view[pos]]` 两层映射（`Table.cpp:769,381`）；owner-data 化只在绘制层按区间取数，不改 `AppendRow/SetFilter/SortBy` 语义。筛选/排序后 `m_view` 即索引映射表（`Table.cpp:381,429`）。
4. **对象常驻**：字体随列/字号缓存（现每帧 `CreateFontW` `Table.cpp:732-735`）；纯色刷由调色板/节点原语提供，不再每行 `CreateSolidBrush`（L774,792）；并入 D2D 表面后改走 `XamlCanvas` 原语（对照 S2 地图 §3.1 清单）。

### 5.2 最小改动面与判据

| 判据 | 目标 |
|---|---|
| 灌行次数 | 每帧绘制行数 = `VisibleCount()`（+ 表头 1 行），不是 `ViewRows()`；未进入视口的行不取值、不画格 |
| 每帧 GDI 对象创建数 | 并入表面前的过渡口径：`CreateCompatibleDC/CreateCompatibleBitmap/CreateFontW/逐行 CreateSolidBrush` 每帧 **=0**（缓存常驻）；最终节点化后 GDI DC/Bmp/Brush/Font 全部退出表格路径 |
| 语义不回退 | 筛选/排序/选中/`EnsureVisible`/滚轮结果与现状一致；`_probe_scroll80.py` 类用例保持 PASS（`_perf_newtable.txt:39-40`） |
| 数据上限 | 不在 owner-data 阶段强加行数上限；大表只受可视区间限制，**待实测**万行级内存/筛选耗时 |

## 6. ③ 滚动闪烁判定与隔离规格

### 6.1 现有证据链

- 表格自带 GDI 双缓冲：内存 DC + `FillRect` 白底 + 一次 `BitBlt`（`Table.cpp:723-727,819`）；`WM_ERASEBKGND` `return 1`（`Table.cpp:631-632`）。
- 滚轮 `ScrollBy` → 钳制 → `Invalidate`（`Table.cpp:705-709,582-589`）。
- 前序判定（S2 地图 §4.2）：新表内部离屏双缓冲 + 擦底抑制，不构成「画两遍」或「漏覆盖」；残留风险为每帧 GDI churn 与父窗连带失效，证据不足标待实测。

### 6.2 并入节点单表面后消失路径

- 节点化后：表格并入表面帧，GDI 离屏 DC/Bitmap（`Table.cpp:724-725,819,822`）、每帧字体（L732-735）、逐行刷（L774,792）退出；行/格改走表面 `XamlCanvas` 原语；整帧后台 buffer 完成后一次 Present（对照 S3 规格 §2.4 机制）。
- 若仍闪：按脏矩形隔离排查。

### 6.3 可验证判据

| 检测 | 方法 | 判定 |
|---|---|---|
| 残影检测 | 连续滚动 N 帧（建议 N≥20，覆盖 ≥3 屏）逐帧抓像素/离屏快照，帧间差按行带比对旧行像素 | 无旧行残留 = 通过；出现横向旧行带 → 隔离失效 |
| 脏矩形越界 | 记录表格节点脏矩形与 `m_scroll` 区间；核对脏矩形是否只覆盖行区间 + 表头 + 滚动条 | 脏矩形不得越出表格节点 `rc`；不得连带兄弟节点/父窗整窗标脏（对照 `moveMany` 并集口径 `PyMfcHost.cpp:3115-3121`） |
| 擦底消息 | 滚动段内 `WM_ERASEBKGND` 计数 | 节点化后应为 0；若仍有子窗擦底，回到 HWND 覆盖层清单逐项核 |
| GDI churn 计数 | 探针统计每帧 `CreateCompatibleDC/CreateCompatibleBitmap/CreateFontW/CreateSolidBrush` 调用 | 目标 0（§5.2）；非 0 则 churn 未清，闪烁可能来自此 |

## 7. ⑤ 滚动条规格（拖拽补丁 + 空态判据 + 水平滚动算术）

### 7.1 现状（复核行号）

- 垂直自绘 track+thumb：`Table.cpp:798-814`（`sbW=10` L799；track L800-803；`MaxScroll>0` 算 `thumbH/thumbY` L804-809；thumb L810-813）。
- 左键分支：表头 `Table.cpp:639-650`；行/格 `Hit` L651-656；**无 track/thumb 命中、无捕获、无 `WM_VSCROLL`**；样式无 `WS_VSCROLL/WS_HSCROLL`（`Table.cpp:228`）。
- 滚轮 `Table.cpp:705-709`；`EnsureVisible` `Table.cpp:591-601`；宿主 `tableEnsureVisible` `PyMfcHost.cpp:1178`。

### 7.2 两种情形判据（先诊断再补丁）

| 情形 | 判据 | 处置 |
|---|---|---|
| A 拇指不出（`MaxScroll==0`，本就不够滚） | `MaxScroll()==0`（`Table.cpp:539-544`）；`ViewRows()≤VisibleCount()`；L805 条件不成立，无 thumb 矩形；滚轮被 `m_scroll>mx` 钳回（`Table.cpp:584-587`） | 正常态，**不补拖拽** |
| B 拇指存在但拖不动 | `MaxScroll()>0`；thumb 像素已画（L810-813）；左键落在 track/thumb 几何内时 `m_scroll` 不变、无 `Invalidate`；滚轮仍可滚 | 按 7.3 补拖拽 |

### 7.3 拖拽补丁精确位置与新成员

- **命中位置**：`Table.cpp` 左键分支区段 `635-656`，在表头/行命中之前先做 track/thumb 命中（几何取自 L799-809 同一组常量与算式）：点 thumb 进入拖拽；点 track 空白按翻页步进（步长口径待对齐 HTML 原型，S3 规格 §4.3 已列待实测）。
- **捕获**：命中后 `SetCapture`（对照画布拖拽捕获 `PyMfcProcs.cpp:2214` 模式）；`WM_MOUSEMOVE` 按拇指位移反算 `m_scroll`；`WM_LBUTTONUP` 释放捕获。
- **`Table.h` 需新增成员**（规格字段）：拖拽标志、起始 `m_scroll`、起始鼠标 y、thumb 几何缓存；复用现有 `m_scroll`（`Table.h:133`）与 `VisibleCount/MaxScroll/ScrollPos`（`Table.h:91-95`）。
- **一致性**：反算后的 `m_scroll` 必须过 `[0, MaxScroll()]` 钳制（与 `ScrollBy` `Table.cpp:584-587` 同源）；重绘走 `Invalidate`（`Table.cpp:588` 同款）；不另立偏移、不新增滚动事件回调（沿用 `Table.h:59,150` 事件表）。

### 7.4 水平滚动是否需要（算术）

- 默认列宽 `COLW=[78,100,70,130,52,100,56,26]`（`pp_app.py:103`），新表取前 7（`pp_app.py:1057`）：
  - 列宽和 = `78+100+70+130+52+100+56 = 586`。
- 表格客户宽 = `ED_W - 2*ED_PAD = 640 - 2*10 = 620`（`pp_app.py:53,60,1056`）；垂直条占 `sbW=10`（`Table.cpp:799`），内容宽 `620-10=610`。
- 余量 = `610-586 = 24 px > 0` → **默认宽度下无水平溢出，不需要水平滚动条**。
- 例外（待实测）：引擎 `rules.table.widths` 覆盖 `COLW`（`pp_app.py:103`）；侧板最小宽 `SPLIT_MIN_W=360`（`pp_app.py:57`）时内宽 `360-20=340`、内容 `330 < 586`，新表 `SetColumns` 不缩放（`Table.cpp:509-515`），窄侧板会横向裁切；旧 ListView 路径按 `gridColumns` 比例缩放（`pp_app.py:607-614`）。溢出时优先列宽比例缩放，不先上水平滚动条。

## 8. S5 施工顺序与回退点

| 步 | 内容 | 前置 | 回退点 |
|---:|---|---|---|
| S5-1 | 诊断 ⑤ 属情形 A/B（§7.2）；采集默认/窄侧板/引擎覆盖列宽下的水平溢出算术（§7.4） | 无 | 不改码，纯探针 |
| S5-2 | 表格对象常驻：字体/纯色刷缓存，退出每帧 GDI 创建（§5.1-5.2） | S5-1 | 开关级回退到每帧建对象；不碰绘制布局 |
| S5-3 | 可视行区间取数（§5.1） | S5-2 | 回退到 pos=0 全量循环（`Table.cpp:766-771`） |
| S5-4 | 情形 B 拖拽补丁（§7.3） | S5-1 确认 B | 移除左键 track/thumb 分支与新成员；滚轮/`EnsureVisible` 不受影响 |
| S5-5 | 表格节点化并入表面（依赖 S2/S3 节点树与表面帧） | S2 节点基础 + S3 详情卡路径 | 表格回退独立 HWND 自绘；保留 HWND 覆盖层策略不变 |

## 9. S4/S5 与 S2/S3 的依赖关系

| 后续步 | 依赖前序 | 说明 |
|---|---|---|
| S4 脏标记/节点矩形衔接 | S2 节点树 `rc/visible/dirty` 字段（现状雏形 `PyMfcHost.h:407-417`）；S1 表面帧 `BeginFrame/EndFrame` | 无节点树时，S4 只在 HWND 层做 resize 合并与并集标脏，不引入节点脏队列 |
| S4 泵内统一出帧 | S1 `PYARX_SURFACE=1` 表面路径；`pump` 语义保持（`PyMfcBind.cpp:603-637`） | `PYARX_SURFACE=0` 回退旧 GDI 链对照 |
| S5-2/5-3 对象常驻 + 可视区间 | 不依赖 S2/S3，可在现有 `TableControl` HWND 内先行 | 先降 churn，再谈节点化 |
| S5-5 表格节点化 | S2 转节点判定（S2 地图 §2 表 #8）；S3 详情卡节点绘制顺序与 z 值口径（S3 规格 §2.3） | 表格输入/原位编辑保留覆盖层 |
| S5 ③ 单表面消除 | S1 表面 + S2 节点化；GDI 双缓冲路径随节点化退出 | 过渡口径下仅做 churn 清理，闪因待 §6.3 判据复测 |
| ④ fit 补丁 | S3 规格 §3；与 S4 视口宽口径对齐（`viewArea` 实际客户区，`pp_app.py:1175-1178`） | S4 改 viewArea 取数前先落 S3 钳制口径，避免双改动叠加 |

## 10. 待实测清单

1. 440 处 `pump(` 中「等待断言」与「出帧等待」分类；历史 371 口径差集。
2. `BeginPaint/EndPaint` 尾部行号复核（`Table.cpp:720` 对应尾部 `EndPaint`）。
3. 现状大表（万行级）筛选/绘制耗时；owner-data 后复测。
4. 引擎覆盖列宽与窄侧板下的实际横向裁切程度（§7.4 例外）。
5. 滚动条 track 翻页步长的 HTML 原型口径（§7.3）。
6. 表面交换链与保留子 HWND 共存判据（沿用 S2 地图 §6.4，S4/S5 落地前先过）。
