# MFCXaml 渲染重构 S2：代码地图 + 迁移规格

> 口径：本文为**只读分析**，未编译、未启动 GUI、未杀进程、未改任何源文件/配置。
> 所有结论按 `文件:行号` 给证据；证据不足处写「待实测」，不做猜测。
> 工程根：`G:\PyArx`；宿主源码：`PyMfcXaml\PyMfcHost.cpp / PyMfcProcs.cpp / Table.h / Table.cpp`；
> 渲染库：`MfcXamlCore\XamlDraw.h / XamlD2D.h / XamlSurface.h / XamlSurface.cpp / PyMfcSkin.h`；
> Python 应用：`tests\mfc_xaml\pp_app.py`。
> 行号以当前磁盘文件为准；本工程非 git 仓库，无回滚基线。

---

## 0. 现状一句话与 S2 目标对齐

- 现状：宿主窗口 `PyMfcNativeWindow`（`PyMfcHost.cpp:25-32`）+ 一批子 HWND；绘制分三条链：
  ① owner-draw 子窗（STATIC/BS_OWNERDRAW）经父 `WM_DRAWITEM` 直绘；② 皮肤接管子窗经自身 `WM_PAINT` + 离屏 DIB 双缓冲；③ 自注册 `PyMfcTable` 自带 GDI 双缓冲。
- S1 设备层已落地：`IXamlSurface`（D3D11 + DXGI FLIP_DISCARD 交换链 + ID2D1DeviceContext，`XamlSurface.h:33-74`、`XamlSurface.cpp:35-301`）；宿主 `PYARX_SURFACE=1` 时只做不透明清屏 + Present，子 HWND 仍覆盖在上层（`PyMfcProcs.cpp:1079-1101`）。
- S2 目标：宿主只保留一个渲染表面；自绘控件从 HWND 降级为保留树里的绘制节点；原生 HWND 只留真正要输入/IME/下拉列表的（Edit、ComboBox 列表、旧 ListView 兜底等）作覆盖层。

---

## 1. 当前绘制链全图

### 1.1 入口层级与各环节 HDC / 位图 / 裁剪 / z 序 / 擦底

| 环节 | 位置（文件:行号） | HDC 来源 | 位图缓存 / 双缓冲 | 裁剪方式 | z 序处理 | WM_ERASEBKGND 处理 |
|---|---|---|---|---|---|---|
| 宿主创建 | `PyMfcHost.cpp:22-41` | — | — | 宿主样式 `WS_CHILD \| WS_VISIBLE \| WS_CLIPCHILDREN \| WS_CLIPSIBLINGS`（`PyMfcHost.cpp:30`），子窗区域由父裁掉、禁止父跨区作画（注释 `PyMfcHost.cpp:27-29`） | 子窗按创建次序 + `bringToFront`/`m_surfaceOrder` | — |
| 宿主 `OnPaint`（旧 GDI 路径） | `PyMfcProcs.cpp:1071-1119` | `CPaintDC dc(this)`（`PyMfcProcs.cpp:1073`） | 无离屏；先 `FillSolidRect` 整窗底色（`PyMfcProcs.cpp:1107`），再按 `m_backdrops` 逐块 `DrawBackplate`（`PyMfcProcs.cpp:1108-1118`） | `IntersectRect` + `SaveDC/IntersectClipRect/RestoreDC`（`PyMfcProcs.cpp:1110-1117`） | 容器底色先于子窗；`m_surfaces` 装饰矩形在擦底阶段压在子 HWND 之下（`PyMfcProcs.cpp:182-193`） | 见下 |
| 宿主 `OnEraseBkgnd` | `PyMfcProcs.cpp:153-195` | `pDC` 传入 | 无 | `m_backdrops` 块用 `IntersectClipRect`（`PyMfcProcs.cpp:171-178`）；`m_surfaces` 用 `IntersectRect`（`PyMfcProcs.cpp:187-192`） | 装饰矩形先于子 HWND 上屏（注释 `PyMfcProcs.cpp:180-182`） | 新表面路径直接 `return TRUE`，擦底交给交换链后台 buffer Clear（`PyMfcProcs.cpp:155-157`）；旧路径 `FillSolidRect` 整窗 + backdrop + surfaces（`PyMfcProcs.cpp:165,177,192`） |
| 宿主表面路径 `OnPaint` | `PyMfcProcs.cpp:1079-1101` | 表面内部 `ID2D1DeviceContext`（`XamlSurface.h:29`） | 后台 buffer 由 `BeginFrame` 不透明 `Clear`（`XamlSurface.cpp:66-79`）；`EndDraw + Present`（`XamlSurface.cpp:82-104`） | 由表面整帧承担；节点裁剪 S2 再建 | 原生子 HWND 仍各自 `WM_PAINT` 覆盖在上层（注释 `PyMfcProcs.cpp:1079-1080`） | 表面路径 `OnEraseBkgnd` 已吃掉（`PyMfcProcs.cpp:156-157`） |
| owner-draw 子窗安装 | `PyMfcHost.cpp:795-816` | — | — | 子窗创建统一带 `WS_CLIPSIBLINGS`（`PyMfcHost.cpp:778-780` 注释：无它兄弟不裁剪、叠放次序失效） | owner-draw 剥 `WS_BORDER`（`PyMfcHost.cpp:800-808`）；原类过程存 `kOldProcProp`（`PyMfcHost.cpp:811-815`） | — |
| owner-draw 子窗 `WM_PAINT` | `PyMfcHost.cpp:427-456` | `BeginPaint(hwnd, &ps)`（`PyMfcHost.cpp:430`）；自造 `DRAWITEMSTRUCT.dis.hDC = hdc`（`PyMfcHost.cpp:451`） | **此链无离屏 DIB**：D2D 绑到 BeginPaint 原始 HDC 直绘（见下行） | 沿用 BeginPaint 更新区；`paintSkinControl` 链另有 `SelectClipRgn(hdc,nullptr)` 清裁剪（`PyMfcProcs.cpp:1163`），owner-draw 链未见等价清裁剪（待实测） | 由父 `SendMessage(WM_DRAWITEM)` 同步绘制（`PyMfcHost.cpp:453-454`） | `WM_ERASEBKGND`：原生类交回原过程（`PyMfcHost.cpp:371-384`，注释 `373-375`：系统 ListView 自带双缓冲）；其余类经父 `WM_CTLCOLORSTATIC` 取刷 `FillRect` 整块（`PyMfcHost.cpp:388-401`），`return 1`（`PyMfcHost.cpp:402`） |
| 父 `OnDrawItem` | `PyMfcProcs.cpp:1756-1812` | `dc.Attach(lp->hDC)`（`PyMfcProcs.cpp:1761-1762`） | 按控件 id 复用一个 `D2DDcCanvas`（`PyMfcProcs.cpp:1796-1799`），`bind(hdc, 0,0,w,h)`（`PyMfcProcs.cpp:1804`） | `lp->rcItem` 为裁剪基准；画布分支 `drawPlanCanvas` 内部先整块填底（`PyMfcHost.cpp:3827`） | 各控件顺序按 `WM_DRAWITEM` 派发；画布/分割线/底板/文字依次 `return` 分支（`PyMfcProcs.cpp:1769-1816`） | — |
| 皮肤接管子窗安装 | `PyMfcHost.cpp:829-863` | — | — | ListView 表头内部子窗单独接管（`PyMfcHost.cpp:845-862`） | 原皮肤过程入 `skinOldProcs`（`PyMfcHost.cpp:836-837`） | — |
| 皮肤子窗 `WM_PAINT` | `PyMfcProcs.cpp:1499-1507` | `BeginPaint(hwnd, &ps)`（`PyMfcProcs.cpp:1502`）→ `paintSkinControl(hdc, hwnd)`（`PyMfcProcs.cpp:1505`） | 静态共享 32bpp DIB：`s_memDc/s_memBmp/s_memBits`（`PyMfcProcs.cpp:1174-1177,1210-1232`，负高度自顶向下 `PyMfcProcs.cpp:1220`）；画完 `BitBlt SRCCOPY`（`PyMfcProcs.cpp:1280-1281`） | `SelectClipRgn(hdc,nullptr)` 清掉更新区裁剪整块重画（`PyMfcProcs.cpp:1161-1163`，注释：不清会留旧边框/滚动条像素） | 底板/文字在同一 DIB 内叠；`layerAlpha` 走 D2D 图层（`PyMfcProcs.cpp:1273-1278`） | `WM_ERASEBKGND` 直接 `return 1`（`PyMfcProcs.cpp:1509-1510`），擦底由离屏 DIB 承担 |
| 阴影底板缓存 | `PyMfcProcs.cpp:1179-1208,1283-1317` | 首轮 D2D 画进独立 `ShadowCache` DIB（`PyMfcProcs.cpp:1290-1305`）；命中后只 `BitBlt`（`PyMfcProcs.cpp:1199`） | `s_shadowCache[id]` 按尺寸命中（`PyMfcProcs.cpp:1195`） | 整窗缓存矩形 | 阴影层被当纯静态装饰；状态/交互不在此窗 | — |
| 新表格 `PyMfcTable` | `Table.cpp:211-236,717-821` | `BeginPaint(m_hwnd,&ps)`（`Table.cpp:720`） | 每帧 `CreateCompatibleDC` + `CreateCompatibleBitmap` + `SelectObject`（`Table.cpp:724-726`），`FillRect` 白底进内存 DC（`Table.cpp:727`），最后 `BitBlt SRCCOPY`（`Table.cpp:819`）；字体每帧 `CreateFontW`（`Table.cpp:732-735,817`） | 行矩形 `RowRect` 自然裁剪；`rr.top>=rc.bottom` 提前 `break`（`Table.cpp:771`）；未见 `clipPush` 级嵌套裁剪 | 自注册类、自带 `WS_CLIPSIBLINGS`（`Table.cpp:228`）；不经过宿主 `createControl` | `WM_ERASEBKGND` `return 1`（`Table.cpp:631-632`） |
| D2D 后端两类目标 | `XamlD2D.h:983-1040`（HwndRenderTarget，`CreateHwndRenderTarget` L1021，软件回退 L1023-1028）；`XamlD2D.h:1045-1105`（DCRenderTarget，`CreateDCRenderTarget` L1088，`BindDC` L1061/L1096） | 外部 HDC 或窗口 | Hwnd 目标自带双缓冲；DC 目标直接落在外部 DC/离屏 DIB 上 | D2D `PushAxisAlignedClip/Popped`（`XamlD2D.h:454-466`） | — | — |
| 出图离屏（导出 PNG 旁证） | `PyMfcOffscreen.cpp:64` 复用 `drawPowerPlanContent` 到离屏画布 | — | 离屏目标 | — | — | — |

### 1.2 两条 paint 派发关系（避免遗漏）

- 宿主 `WM_PAINT`（`PyMfcProcs.cpp:1071`）与控件 `WM_PAINT`（`PyMfcHost.cpp:427` / `PyMfcProcs.cpp:1499` / `Table.cpp:633-634`）各自 `BeginPaint/EndPaint`，彼此不共享 HDC；`WM_DRAWITEM` 是 owner-draw 控件被父同步回调的二次绘制入口（`PyMfcHost.cpp:453-454` → `PyMfcProcs.cpp:1756`）。
- `WM_PRINTCLIENT` 被两条链各接一份，供验收脚本取像素（owner-draw `PyMfcHost.cpp:404-425`；皮肤 `PyMfcProcs.cpp:1512-1521`）。S2 节点化后，截图路径也要改由表面节点树出图，不能依赖 `WM_PRINTCLIENT`。

---

## 2. 自绘控件清单与「转节点 / 保留 HWND 覆盖层」判定

### 2.1 分类判定表（按当前代码路径）

| # | 类 / 注册类 | 创建入口（文件:行号） | 绘制路径 | 输入依赖 | 判定 | 理由与必须先改的依赖点 |
|---|---|---|---|---|---|---|
| 1 | `STATIC` + `SS_OWNERDRAW`（标签） | `PyMfcHost.cpp:875-878` | OwnerDrawProc → 父 `OnDrawItem` 文字/外观分支（`PyMfcProcs.cpp:1819-1892`） | 无（展示用） | **转 scene node** | 纯声明式外观；依赖点：`XamlAppearance` 查找（`PyMfcProcs.cpp:1767,1809`）、`findIcon`（`PyMfcProcs.cpp:1869`）、`m_labelFg/m_accent` 配色表、`xamlDrawText`（`XamlDraw.h:227-245`）；S2 需把这些并入节点绘制闭包 |
| 2 | `STATIC` + `SS_WHITERECT`（白板面板） | `PyMfcHost.cpp:1219-1222` | 原生 STATIC 自绘；另有 `m_surfaces` 装饰矩形在擦底阶段单独铺底（`PyMfcProcs.cpp:182-193,1108-1118`；`setSurface` `PyMfcHost.cpp:3119-3129`） | 无 | **转 scene node** | 纯色块；依赖点：`surfaceColorAt` 底色命中（`PyMfcProcs.cpp:1776-1779,1819`）、`m_surfaceOrder` z 序表；节点化后这块擦底矩形并入表面底板层 |
| 3 | `PyMfcSkinBox`（`kSkinBoxShadow` 阴影底板） | `PyMfcHost.cpp:1335-1343`（`addShadowBox`）；类注册 `PyMfcHost.cpp:157-171`；皮肤过程 `PyMfcProcs.cpp:256-260` | 皮肤链 `paintSkinControl`；阴影缓存 DIB（`PyMfcProcs.cpp:1192-1208,1283-1317`）；`pymfcskin::drawShadowBox`（`PyMfcSkin.h:1045-1060`） | 无（装饰；鼠标是否穿透由调用方决定，`PyMfcHost.cpp:1338-1343` 注释） | **转 scene node**（底板 + 阴影并入画布 D2D 层） | 依赖点：`m_shadowBox[id]` 风格表、`ShadowBoxStyle`（`PyMfcSkin.h:1021-1033`）、阴影多层扩散算法；详情浮层底板即此类（性能背景见 `_perf_newtable.txt:14-15,38`） |
| 4 | 配电画布 `PowerCanvas`（`STATIC SS_OWNERDRAW`） | `PyMfcHost.cpp:3474-3490`（`m_planCv[id]` 先登记 `L3477`） | OwnerDrawProc → `OnDrawItem` 画布分支 `drawPlanCanvas`（`PyMfcProcs.cpp:1769-1775`；`PyMfcHost.cpp:3821-3911`）；内容 `drawPowerPlanContent`（`PyMfcCommon.h:71-89`） | 鼠标 down/move/up/wheel 平移缩放，`PyMfcProcs.cpp:2159-2259` | **转 scene node**（画布本体）；节点命中测试接管现有鼠标状态机 | 依赖点：`PlanCanvasState` 全套（`PyMfcHost.h:516-539`：`mode/zoom/ox/oy/sel/drag/down/mini/detail*`）、小地图双向引用 `miniOf`（`PyMfcHost.cpp:3865-3883`）、选中/详情弹层 `detailRc/detailTexts`（`PyMfcHost.cpp:3888-3910`）；平移缩放状态机要从 HWND 消息搬到节点 hit-test |
| 5 | 分割线 `addStaticLine`（`STATIC SS_OWNERDRAW`） | `PyMfcHost.cpp:1245-1249`（登记早于建窗，注释 `1242-1244`） | `OnDrawItem` `m_lines` 分支（`PyMfcProcs.cpp:1781-1816`） | 无（细分隔线）；**可拖动分隔条**另有 `PyMfcSplitterProc`（`PyMfcHost.cpp:1235-1237,227-751`），是另一个 HWND 状态机 | 静态线 **转节点**；可拖动分隔条先保留 HWND 覆盖层（或随 SplitPane 复合控件再节点化） | 依赖点：`m_lines` 登记表、横竖条判别（`PyMfcProcs.cpp:1793-1808`）；分隔条拖动状态机（`PyMfcHost.cpp:230-751`）若节点化需先补节点级命中/拖拽捕获 |
| 6 | `BUTTON` + `BS_OWNERDRAW`（工具/详情/编辑器按钮） | `PyMfcHost.cpp:870-873` | OwnerDrawProc → `OnDrawItem` 底板/文字/图标分支（`PyMfcProcs.cpp:1823-1892`）；按下态 `BM_GETSTATE`（`PyMfcHost.cpp:441-448`） | 点击（`BN_CLICKED`）、焦点 | **转 scene node**（视觉 + 节点命中派发点击） | 依赖点：`m_btn/m_border/m_accent` 配色、`xamlDrawBackplate`（`XamlDraw.h:166-225`）、`BM_GETSTATE` 按压/焦点态；S2 需节点级 hover/pressed 状态与点击回调桥到现有 Python 回调 |
| 7 | `BUTTON` + `BS_AUTOCHECKBOX`（复选框） | `PyMfcHost.cpp:890-893` | 勾选块在 `OnDrawItem` 复选/单选分支（`PyMfcProcs.cpp:1894-1896` 注释：owner-draw 接管后原生勾选标记不上屏） | 点击切换状态 | **条件转节点**：先核清它是否真走 owner-draw 派发（`isNativePaintClass`/`isSkinPaintClass` 均不含 `BUTTON`，`PyMfcHost.cpp:214-217,183-197`；`OwnerDrawProc` 仅在 `SS/BS_OWNERDRAW` 时安装 `PyMfcHost.cpp:795-815`）；绘制链路归属**待实测** | 依赖点：勾选绘制分支（`PyMfcProcs.cpp:1894+`）、`setCheck` 状态回写；若保留 HWND 也合理（输入控件语义强） |
| 8 | 新表格 `PyMfcTable`（自注册 HWND） | `Table.h:71-123`；`Table.cpp:208-236` | 自身 `WndProc`/`OnPaint`（`Table.cpp:625-821`）；数据/命中/滚动/筛选/排序全在控件内（`Table.h:78-119`） | 单击/双击/选中/表头筛选/右键菜单/键盘/滚轮（`Table.cpp:635-710`） | **候选转 scene node（建议 S3）**：视觉 + 数据可入保留树，命中与滚动先随节点；输入框（原位编辑）另留覆盖层 | 依赖点：`m_rows/m_colW/m_colTitle/m_view/m_scroll/m_sel/m_colType/m_filter`（`Table.h:128-136`）、`Hit/RowRect/Cell`（`Table.h:86-88`）、`ScrollBy/EnsureVisible`（`Table.h:91-95`，实现 `Table.cpp:532-600`）、`SetCallback` 事件表（`Table.h:59,119,150-151`）；**注意：hint 称新表"已经不是 HWND"，当前源码证据相反——它是独立注册窗口类的真 HWND（`Table.cpp:218,228`），只是不经宿主 `createControl`** |
| 9 | `EDIT`（文本输入） | `PyMfcHost.cpp:880-883`；皮肤/原生双清单都含 `Edit`（`PyMfcHost.cpp:194,217`） | 皮肤链 `paintSkinControl` → `pymfcskin::drawEdit`（`PyMfcProcs.cpp:1066`）；系统保留光标/IME | 键盘输入、光标、IME、剪贴板 | **保留 HWND 覆盖层** | 依赖点：caret 闪烁状态 `caretBlinkStates`（`PyMfcProcs.cpp:145-155,1524+`）；节点化不取消输入窗 |
| 10 | `COMBOBOX`（下拉） | `PyMfcHost.cpp:921-925` | 皮肤/原生清单重叠（`PyMfcHost.cpp:185,216`）；下拉列表是独立系统子窗 | 展开列表、选项点击 | **保留 HWND 覆盖层** | 下拉子窗的层级与裁剪不能由画布节点代劳 |
| 11 | 旧 `SysListView32` 兜底表 / `ListBox` / `SysTreeView32` / `SysTabControl32` / `SysDateTimePick32` / `SysMonthCal32` / `SysLink` / `RICHEDIT50W` | 旧表 `PyMfcHost.cpp:996-1004`；清单 `PyMfcHost.cpp:185-197,214-218` | 原生过程自绘（`PyMfcHost.cpp:362-368`）；皮肤子链仅换底色/文字 | 各自原生交互 | **保留 HWND 覆盖层**（兜底/遗留，S2 不优先动） | 旧表带 `LVS_EX_DOUBLEBUFFER`（`PyMfcHost.cpp:1003-1004`）；S2 节点树落地前不要拆 |

### 2.2 pp_app 实例级计数（按 `pp_app.py` 当前源码清点）

| 类别 | 实例数 | 出处（行号） |
|---|---|---|
| 标签 STATIC 展示 | 14 | `L_HINT` 407、`L_STATUS` 424、`DT_TITLE` 971、`DT_L0`×4 978、`DT_LV` 987、`DT_LL` 991、`DT_LLOAD` 995、`DT_TIP` 1016、`L_EDIT_TITLE` 1046、`L_EDIT_COUNT` 1047、`L_EDIT_FOOT` 1075 |
| 白板面板 | 2 | `ST_PANEL` 423、`ED_PANEL` 1032 |
| 阴影底板 SkinBox | 2 | `MI_PANEL` 429、`DT_PANEL` 965 |
| PowerCanvas | 2 | `CV` 420、`MINI` 431 |
| 静态分割线 | 1 | `SPLIT` 1036 |
| BS_OWNERDRAW 按钮 | 24 | 工具行 15：`B_DOC/B_3L/B_EDIT/B_CSV/B_XLSX/B_JSON/B_PASTE/B_EJSON/B_ECSV/B_ESVG/B_EPNG` 383-395、`B_ZOUT/B_Z100/B_ZIN/B_FIT` 399-402；详情 3：`D_SAVE/D_ADDCHILD/D_DEL` 1008-1012；编辑器 6：`B_CLOSE/B_APPLY/B_ADDROW` 1049-1051、`B_LOADROW/B_APPLYROW/B_DELROW` 1070-1072 |
| 新表格 `PyMfcTable` | 1 | `TBL` 1056（`self._new_table` 开关 296/323；旧 ListView 兜底 1061） |
| 复选框（条件判定，未计入上列转节点） | 1 | `C_CABLE` 403 |
| 保留 HWND（不计入转节点） | — | `E_TITLE/E_SEARCH` 380/416、详情 4 个 `E_DF0` 980、`D_LEN/D_LOAD` 993/997、行编辑 7 个 `E_F0` 1068；`C_HORIZ/D_VOL` 410/989 |

**小结：明确判「转 scene node」的为 7 类、46 个 pp_app 实例（标签 14 + 白板 2 + 阴影 2 + 画布 2 + 静态线 1 + 按钮 24 + 表格 1）；复选框 1 个链路归属待实测后再归类。** 表格转节点建议排在画布/按钮/底板之后（S3），因为它自带命中/滚动/筛选状态机，迁移面最大。

---

## 3. 现有绘制原语盘点

### 3.1 `XamlCanvas` 抽象接口（`XamlDraw.h:17-130`）与 D2D 实现签名（`XamlD2D.h`）

| 原语 | 接口签名（`XamlDraw.h`） | D2D 实现位置（`XamlD2D.h` 行号） | 备注 |
|---|---|---|---|
| begin/end | `begin(int w,int h)` / `end()` L21-22 | `begin` L193-216（清 Identity 变换/图层栈兜底）；`end` L218-225（`D2DERR_RECREATE_TARGET` 释放目标） | 节点帧开始/结束的现有钩子 |
| fillRect | `(const RECT&, COLORREF)` L24 | L227-232 | |
| fillRectAlpha | `(const RECT&, COLORREF, int alpha)` L28-32（默认退化不透明） | L234-246 | 阴影/半透明叠层 |
| fillRoundRect | `(const RECT&, face, border, radius)` L34 | L248-280（同色不描边只填一次；1px 描边内缩 0.5px） | |
| fillEllipse | `(const RECT&, c, border)` L35 | L282-303 | |
| strokeRect | `(const RECT&, c, thickness)` L36 | L305-317（逐像素条带） | |
| strokeRoundRect | `(const RECT&, c, radius, thickness)` L38-41（默认退化直角） | L319-331 | |
| strokeLine | `(const POINT& a,b, c, thickness)` L42 | L333-339 | |
| frame3d | `(const RECT&, base, sunken, thickness)` L43 | L341-361 | Sunken/Raised 3D 边 |
| text | `(const RECT&, wstring, fg, align, ptSize, family)` L46-47 | L388-429（DWrite，`D2D1_DRAW_TEXT_OPTIONS_CLIP` L428） | |
| textWidth | `(wstring, ptSize, family) -> float` L52-56（缺能力返 -1） | L433-452 | 量尺与 `text` 同格式 |
| clipPush / clipPop | `(const RECT&)` L58-59 | L454-466（`PushAxisAlignedClip/Popped`，计数 `m_depth`） | 节点裁剪区可直接复用 |
| fillRectGradient | `(rc,c1,c2,angleDeg)` L68-72（默认退化单色） | L472-483 | |
| fillRoundRectGradient | `(rc,c1,c2,angleDeg,radius)` L73-77 | L485-496 | |
| fillPath | `(vector<POINT>, c)` L80-90（默认退化包围盒） | L498-505 | |
| strokePath | `(vector<POINT>, c, thickness, closed)` L91-92（默认空） | L507-517（圆头笔帽 `roundStrokeStyle` L520-530） | |
| fillSoftShadow | `(rc,radius,color,alpha,blur,dx,dy)` L96-103 | L532-539+（层数上限 8，L537-538） | |
| pushOpacity / popOpacity | `(int alpha)` / 无参 L107-108（默认空） | L559+ / L590+ | 整块图层 alpha 合成 |
| pushRotate / popTransform | `(angleDeg, center)` / 无参 L111-112（默认空） | L624+ / L635+ | 几何变换栈 |
| drawImage | `(rc, path)` L117（默认空） | L644+（WIC 解码缓存 `m_images` `XamlD2D.h:959`） | 图标/背景图 |
| setTextStyle | `(weight,style,wrapping,valign)` L125-129（默认空） | L364-371；权重映射 `textWeightOf` L373-379 | WPF 对齐语义 |

皮肤级组合绘制（`PyMfcSkin.h`）：`drawShadowBox(XamlCanvas&, const RECT&, const ShadowBoxStyle&)`（`PyMfcSkin.h:1045`）；`drawEdit` L506（被 `PyMfcProcs.cpp:1066` 调用）；`drawSpinBox` L599、`drawRangeSlider` L634、`drawBusyRing` L672；其它 kind 分支按 `PyMfcProcs.cpp:259-264` 的 `m_skinKinds` 反查。

### 3.2 节点注册需要的字段（现状无节点树，以下为 S2 应建字段；标「待新建」）

> 证据：`MfcXamlCore` 内未发现通用节点基类；现有结构仅 `xamlplan::PowerNode`（`XamlPowerPlan.h:13`）与宿主 `PlanCanvasState`（`PyMfcHost.h:516-539`）。字段集对标 S1 表面一帧能力与现有控件状态：

| 字段 | 类型/含义 | 现状对应物（证据） | 对接方式 |
|---|---|---|---|
| `rect` | 父客户区坐标矩形 | 各控件 `GetClientRect`（`PyMfcProcs.cpp:203-204,1157`；`Table.cpp:721`）；`lp->rcItem`（`PyMfcProcs.cpp:1763`） | 节点持有自身矩形；布局仍由现有 `_layout/moveMany`/XAML 定位结果写入 |
| `z` | 叠放次序 | `WS_CLIPSIBLINGS` 兄弟裁剪（`PyMfcHost.cpp:780`）；`m_surfaceOrder`（`PyMfcProcs.cpp:182,3125-3126`）；`bringToFront` | 节点树有序列表 + 显式 z 表；静态底板层 < 控件层 < 浮层层 |
| 绘制回调 | `draw(XamlCanvas&, frame)` 闭包 | `drawSkinControl`（`PyMfcProcs.cpp:200`）、`drawPlanCanvas`（`PyMfcHost.cpp:3821`）、`OnDrawItem` 各分支 | 每个节点挂现有绘制函数；入参改为节点 `rect/state`，HDC 由表面帧提供，不再 `BindDC` 到 BeginPaint DC |
| 脏标记 / 脏矩形 | `needPaint` + `dirtyRect` | `InvalidateRect` 调用点：平移 `PyMfcProcs.cpp:2235,2202`；表格滚轮/选中 `Table.cpp:705-709` 后 `Invalidate`；宿主 `setSurface` `PyMfcHost.cpp:3128` | S2 节点级脏矩形合并到帧更新区；脏标记替代反复整窗 `Invalidate` |
| 裁剪区 | 嵌套 clip rect 栈 | `clipPush/clipPop`（`XamlD2D.h:454-466`）；GDI `IntersectClipRect`（`PyMfcProcs.cpp:176,1115`）；小地图 `IntersectRect`（`PyMfcHost.cpp:3880`） | 节点绘制前 `clipPush(rect)`，结束 `clipPop`；画布内子元素沿用 `RowRect/Cell` 几何 |
| 父坐标变换 | 父→本节点偏移 / 缩放（画布 `zoom/ox/oy`） | `PlanCanvasState.zoom/ox/oy`（`PyMfcHost.h:521-522`）；`drawPowerPlanContent` 的 `X/Y/PX` lambda（`PyMfcCommon.h:77-79`） | 画布节点把变换存在自身状态；子节点按父变换累计；非画布节点恒等变换 |
| 状态/外观句柄 | 外观声明、配色、选中/拖拽态 | `XamlAppearance` 查找 `PyMfcProcs.cpp:1767,209-249`；`m_accent/m_labelFg/m_ctrlBack`（`PyMfcProcs.cpp:236-239,1820-1822,1856`）；`PlanCanvasState.sel/drag/moved` | 节点持有浅状态；XAML 外观表以 id 为键保留，迁移期不重做 |
| 输入回调 | 命中/点击/聚焦派发 | 控件过程存 `kOldProcProp`（`PyMfcHost.cpp:815`）；表格 `TableEventFn`（`Table.h:59,119`）；画布消息状态机 `PyMfcProcs.cpp:2159-2259` | 覆盖层 HWND 继续走消息；节点命中在表面客户区坐标做矩形/几何判定，再桥到 Python 回调 |

### 3.3 与 `PaintStruct` / `pymfcskin` 皮肤函数的对接

- 现有 `PAINTSTRUCT ps` 只在三条 HWND 绘制入口出现（`PyMfcHost.cpp:429-430`、`PyMfcProcs.cpp:1501-1502`、`Table.cpp:719-720`）；节点化后不再需要 `BeginPaint/EndPaint`，帧边界由 `IXamlSurface::BeginFrame/EndFrame`（`XamlSurface.h:40,44,48`）替代。
- `pymfcskin::drawEdit` 等皮肤函数入参已是 `XamlCanvas& + 几何 + Palette`（`PyMfcProcs.cpp:241-249,1066`），是最容易平移到节点回调的一批；`drawShadowBox` 同构（`PyMfcSkin.h:1045`）。
- GDI 直画残留点：白板快路径 `FillRect(hdc,&cr,fb)`（`PyMfcProcs.cpp:1249`）、表格整段 GDI `FillRect/DrawTextW`（`Table.cpp:727,741,753,775,789,794,802,812`）——节点并入 D2D 表面时这些需改走 `XamlCanvas` 原语，不能继续拿 GDI HDC。

---

## 4. 两条闪烁缺陷的根因判定（②画布平移、③表格滚动）

### 4.1 ②画布平移闪烁

**代码路径（已核行号）：**
1. 拖拽按下：`WM_LBUTTONDOWN` 记 `st.down/downOx/downOy` + `SetCapture`（`PyMfcProcs.cpp:2206-2218`）。
2. 拖动：`WM_MOUSEMOVE` 仅在 `st.moved` 后更新 `st.ox/st.oy`（`PyMfcProcs.cpp:2224-2233`），`InvalidateRect(hwnd, nullptr, FALSE)`（`PyMfcProcs.cpp:2235`）；滚轮缩放同样 `InvalidateRect(...,FALSE)`（`PyMfcProcs.cpp:2202`）。hint 指的 `L2157-2173` 是函数头与调试日志段；真正更新偏移 + 失效在 `L2232-2235`。
3. 画布是 `STATIC SS_OWNERDRAW`（`PyMfcHost.cpp:3478`），经 `OwnerDrawProc`：
   - `WM_ERASEBKGND`：非原生类，经父 `WM_CTLCOLORSTATIC` 取刷 `FillRect` 整块（`PyMfcHost.cpp:388-401`），`return 1`（`PyMfcHost.cpp:402`）。
   - `WM_PAINT`：`BeginPaint` → 造 `DRAWITEMSTRUCT` → 父 `OnDrawItem`（`PyMfcHost.cpp:427-456`）；`D2DDcCanvas.bind` 到该 BeginPaint HDC 直绘（`PyMfcProcs.cpp:1804`），**owner-draw 链没有离屏 DIB**（离屏 DIB 只在皮肤链 `paintSkinControl`，`PyMfcProcs.cpp:1210-1232,1281`）。
4. 绘制内容先整块填底 `#F3F5F8`（`PyMfcHost.cpp:3827`），再画 plan/SVG/minimap（`PyMfcHost.cpp:3828-3886`）。

**判定：偏向「同一区域被画两遍」（先擦底一遍、再 D2D 直绘一遍），不是「一次未覆盖全区域」。**
- 覆盖性证据：`drawPlanCanvas` L3827 整块 `fillRect(rc, ...)` 覆盖整个客户区；未发现漏画边角的分支。
- 两遍性证据：擦底 `FillRect`（`PyMfcHost.cpp:399`）与 D2D 直绘先后落在同一 HDC；中间无后台缓冲隔离，旧帧先被父底色盖住、D2D 再光栅化到屏幕 DC，两拍之间可见底色闪。
- 不确定点：`InvalidateRect(...,FALSE)` 理论上不标擦底；拖动期间 `WM_ERASEBKGND` 是否真实每次触发，**待实测**（可在 `PyMfcHost.cpp:371` 入口加计数探针，但本轮只读未加）。即便擦底消息不触发，owner-draw 链缺离屏缓冲、D2D DC 目标直落屏幕 DC，仍是闪烁来源。

**flip-model 单表面化之后：** 大概率**自然消失**——擦底路径在表面路径已被 `OnEraseBkgnd return TRUE` 吃掉（`PyMfcProcs.cpp:156-157`），整帧在后台 buffer 完成 `BeginDraw→Clear→绘制→EndDraw→Present`（`XamlSurface.cpp:66-104`），不再有「擦底 + 直绘」两拍。
**可验证判据（S2 落地后执行，不预下结论）：**
- 探针计数：拖动平移段内 `WM_ERASEBKGND` 出现 0 次；`BeginPaint` 每帧 1 次、`Present` 每帧 1 次。
- 双帧差：相邻两帧屏幕差里不应出现整块父底色条带；卡片边缘无残影。
- 若仍闪：排查是否兄弟/兄弟板面被连带 `Invalidate`（`PyMfcHost.cpp:730,741` 分隔条拖动会失效父窗），或画布未并入表面、仍是独立 HWND 覆盖层。

### 4.2 ③表格滚动闪烁

**代码路径（新表 `PyMfcTable`）：**
- 滚轮 `WM_MOUSEWHEEL` → `ScrollBy(±1)`（`Table.cpp:705-709`）；选中/表头交互后 `Invalidate`。
- `OnPaint` 自带 GDI 双缓冲：`CreateCompatibleDC/Bitmap` → 内存 DC `FillRect` 白底 → 画表头/行/选中条/滚动条 → `BitBlt` 一次上屏（`Table.cpp:723-819`）。
- `WM_ERASEBKGND` 直接 `return 1`（`Table.cpp:631-632`），不擦底。
- 旧兜底 `SysListView32` 带 `LVS_EX_DOUBLEBUFFER`（`PyMfcHost.cpp:1003-1004`），擦底交原生过程（`PyMfcHost.cpp:373-383`）。

**判定：新表内部已是离屏双缓冲 + 擦底抑制，按现有证据不构成「同一区域画两遍」；也未见「未覆盖全区域」的证据**（整矩形先 `FillRect` `Table.cpp:727`，行越界 `break` `Table.cpp:771`，滚动条 track 整段铺 `Table.cpp:800-803`）。`_trail_a.png/_trail_b.png`（`_diag_seltrail.py:36-51` 产物）为选中行对比截图，两张各只有一行高亮，未见多行蓝残留；`_probe_scroll80.py` 结果见 `_perf_newtable.txt:39-40`（末行可视区内、回顶首行 top=24、下滚后首行 top=-102，PASS 口径）。
**残留风险（证据不足，标待实测）：**
- 每帧 `CreateCompatibleBitmap/DeleteObject` + `CreateFontW/DeleteObject`（`Table.cpp:725-726,732-735,817,820-821`）是 GDI  churn；若表现为滚动抖动而非闪白，可能在此。
- 滚动触发后是否连带父窗/兄弟 HWND 失效，或旧 ListView 路径仍被实际走到（`self._new_table` 判定 `pp_app.py:296`），**待实测**。

**flip-model 单表面化之后：** 表格并入节点单表面后，GDI 离屏 DC/BitBlt 与每帧字体重建一并消失，滚动闪烁**应自然消除**；但若当前闪是父窗连带失效造成，节点化后仍需补节点脏矩形隔离。
**可验证判据：** 滚轮连续滚动期间连续抓 N 帧，行内容无旧行残影、无白底穿透；`InvalidateRect` 仅落在表格节点脏矩形内。

---

## 5. 两个功能缺口根因（④初始视图未 fit、⑤新表滚动条）

### 5.1 ④画布初始视图未 fit（整图放大、左侧被裁）

**初始化/赋值点（行号 + 现状）：**

| 层 | 位置 | 现状值/规则 |
|---|---|---|
| C++ 状态默认 | `PyMfcHost.h:521-522` | `zoom = 1.0f; ox = 0, oy = 0` |
| C++ `getPowerCanvasView` 缺省 | `PyMfcHost.cpp:3607-3616`（缺状态回退 `zoom=0.0f/ox=oy=0`，L3609-3610） | Python 端 `zoom<=0` 走 `or 1.0`（`pp_app.py:1194,1206,1220`） |
| C++ `fitPlanCanvas` | `PyMfcHost.cpp:3453-3472` | `kw/kh` 按客户区减 8px（L3464-3465）；`zoom = min(min(kw,kh), 1.0f)` **封顶 1.0**（L3466）；下限 0.2（L3467）；`ox/oy` 居中后 **`<0 钳回 0`**（L3468-3471） |
| C++ 装数据后自动 fit | `setPowerCanvasPlan` `PyMfcHost.cpp:3511`；`setPowerCanvasScene` `PyMfcHost.cpp:3534`；建窗后 `PyMfcHost.cpp:3484-3486` | 装场景即调 `fitPlanCanvas` |
| Python `rebuild` | `pp_app.py:1136-1143` | `loadPowerScene(CV/MINI)` 后，`fit=True` 调 `self.fitView()`（L1140-1141），否则 `centerKeep()` |
| Python `fitView` | `pp_app.py:1180-1190` | `k = min(aw/W, ah/H) * 0.92`（L1185）；**`k = max(0.2, min(4.0, k))` 上限 4.0、未封顶 1.0**（L1186）；`ox/oy` 居中但**未钳 ≥0**（L1187-1188）；`setPlanView(CV,k,ox,oy)`（L1189） |
| Python `centerKeep` | `pp_app.py:1216-1225` | 沿用现有 zoom 居中；`ox/oy` 同样未钳 ≥0（L1223-1224） |

**根因判定：** C++ `fitPlanCanvas` 先按「最多 100%、偏移不越界」算好视图（`PyMfcHost.cpp:3466,3470-3471`），但 Python `rebuild(fit=True)` 紧接着用 `fitView` 覆盖；`fitView` 允许 `k>1`（模型比视口小时放大到 4.0 上限），且 `ox/oy` 可为负——`k>1` 时 `W*k > aw`，`ox=(aw-W*k)/2 < 0`，左半边被裁出客户区，与「整图放大、左侧被裁」现象一致；`_base_frozen.png` 左侧卡片贴边/被裁可见旁证。`_pp_canvas_fit.png` 是另一组口径下 fit 正常的截图，说明触发依赖模型尺寸/视口比例，**具体复现条件待实测**。

**应改成什么（规格，不在本轮改码）：**
- `pp_app.py:1186` 对齐 C++：`k` 上限封顶 `1.0`（保留 0.2 下限）；`pp_app.py:1187-1188` 的 `ox/oy` 钳 `>=0`，与 `PyMfcHost.cpp:3466-3471` 一致；`centerKeep` 的 `ox/oy` 同样钳 ≥0（`pp_app.py:1223-1224`）。
- 复核 `viewArea()` 用 `CV_W` 还是真实客户区（`pp_app.py:1175-1178`）：编辑器打开时 `used=sideWidth+SPLIT_W`，关闭时 used=0；fit 视口必须与画布实际客户区一致，否则偏移基准错。
- 备选：Python 不再自算 fit，统一调 C++ `fitPlanCanvas` 口径（现有 `setPowerCanvasScene` 已会触发一次，`PyMfcHost.cpp:3534`）；避免两套 clamp 规则漂移。

### 5.2 ⑤新表格「滚动条」缺口

**现状源码证据（与 hint 「还没有滚动条」存在出入，以代码为准）：**

| 项 | 现状（文件:行号） | 结论 |
|---|---|---|
| 垂直自绘 track + thumb | `Table.cpp:798-814`：`sbW=10`、track 矩形 L800、底色 L801-803；`MaxScroll>0` 算 `thumbH/thumbY` L804-809、thumb 矩形 L811、填充 L810-813 | **视觉滚动条已存在**（仅行数超可视区出拇指，注释 `Table.cpp:798`） |
| 滚轮滚动 | `Table.cpp:705-709` `ScrollBy(±1)` | 已接 |
| 滚动状态接口 | `ScrollPos/VisibleCount/MaxScroll/ScrollBy/EnsureVisible`（`Table.h:91-95`；实现 `Table.cpp:532-600`）；宿主 `tableEnsureVisible` 暴露 `PyMfcHost.cpp:1178` 链；Python 选中行/原位编辑/追加行调 `ensureVisible`（`pp_app.py:657-658,771-772`） | 程序内滚动与可见性保证已接 |
| thumb/track 鼠标拖拽 | `WndProc` 左键分支只判表头（`Table.cpp:639-650`）与 `Hit` 行/格（`Table.cpp:651-656`）；**无 track/thumb 命中分支、无拖拽捕获、无 `WM_VSCROLL` 处理**；`Table.h:635-710` 消息表未见滚动条拖拽 | **缺口 A：拇指/轨道不可鼠标拖拽**（只能滚轮/键盘/选中跟随） |
| 水平滚动 | 列宽之和超出时无横向 track/thumb，无 `m_hscroll` 状态；`WS_HSCROLL` 未加（`Table.cpp:228`） | **缺口 B：无水平滚动**（当前 7 列按 `COLW` 排，是否触发超宽待实测） |
| 系统滚动条 | 未用 `WS_VSCROLL`/`WS_HSCROLL`（`Table.cpp:228` 仅 `WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS`） | 与旧 ListView 的系统条不同；新表是自绘条 |

**接线缺口定位：** 宿主 `addTable` 只 `TableControl::Create` + 回调（`PyMfcHost.cpp:1079-1087`），未把滚动条拖拽交互挂给控件；Python 侧无滚动条回调需求（事件只到行/列，`Table.h:57` what 码）。S2/S3 节点化时，滚动条作为表格节点内子元素绘制；拖拽命中可复用表格节点命中测试，不必另建 HWND 滚动条。
**待实测项：** 现场用户看到的「无滚动条」是指拇指不出（`MaxScroll` 算错）、还是拇指存在但拖不动；`_probe_scroll80.py` 已证垂直滚动生效（`_perf_newtable.txt:39-40`），thumb 拖拽与水平滚动未被该探针覆盖。

---

## 6. S2 施工建议顺序、开关矩阵、共存验证判据

### 6.1 开关现状（代码证据）

| 开关 | 读取点 | 行为 |
|---|---|---|
| `PYARX_SURFACE` | `PyMfcHost.cpp:47-50`；`ensureSurface` `PyMfcHost.cpp:43-68` | `=1` 走 `IXamlSurface` D3D11 交换链；缺省/`=0` 回旧 GDI 路径；`Init` 失败 `delete surface` 回退（`PyMfcHost.cpp:55-60`） |
| `PYARX_D2D_RT` | `XamlD2D.h:43-55`（缓存；`software` → WARP 软件，否则硬件）；Hwnd 目标硬件失败再退软件 `XamlD2D.h:1023-1028` | 只控制 D2D 渲染目标类型；不改变 HWND 派发链 |
| 表面内部常量 | `XamlSurface.cpp:258,263,266`：`BufferCount=2`、`FLIP_DISCARD`、`AlphaMode=IGNORE`；`CreateSwapChainForHwnd` L269-270 | 现 S1 后端固定这一组 |

### 6.2 开关矩阵（各组合下当前生效路径；S2 目标态标注）

| `PYARX_SURFACE` | `PYARX_D2D_RT` | 当前生效路径 | S2 节点化后预期 |
|---|---|---|---|
| 0（缺省） | software | 宿主 GDI 擦底/铺底；owner-draw 链 D2D DCRenderTarget 绑 BeginPaint DC（WARP）；皮肤链离屏 DIB + DCRenderTarget；表格 GDI 双缓冲 | 保留为回退路径；表面接口 `Init` 失败时即落此路径（`PyMfcHost.cpp:55-60`） |
| 0 | hardware | 同上，D2D 硬件 DCRenderTarget；Hwnd 目标未在宿主使用（仅预览器/设计器画布，`XamlD2D.h:983`） | 同上 |
| 1 | software | `IXamlSurface` BeginFrame 不透明 Clear + EndFrame Present（`PyMfcProcs.cpp:1081-1101`）；擦底被吃（`PyMfcProcs.cpp:156-157`）；子 HWND 各自旧链覆盖；D2D 子目标仍受 `PYARX_D2D_RT=software` 影响 | S2 主线：画布/按钮/底板/静态线节点绘制落到表面 `ID2D1DeviceContext`；Edit/ComboBox 等覆盖层保留旧链 |
| 1 | hardware | 同上，子 D2D 目标硬件 | 同上；需跑设备丢失/重建探针（`XamlSurface.h:56,60,73`；`PyMfcProcs.cpp:1129-1150`） |

矩阵待补：`PYARX_SURFACE=1` 与子 D2D 目标（DCRenderTarget）并存时的叠加顺序、子 HWND 与交换链的合成层级，当前 S1 代码只注释「原生子 HWND 仍覆盖在上层」（`PyMfcProcs.cpp:1079-1080`），**未见层级显式仲裁代码，待实测**。

### 6.3 建议施工顺序（S2，先易后难、先节点纯装饰后交互）

1. **设备层对接复核（S1 已落，S2 先验）**：`BeginFrame/EndFrame/Resize/IsDeviceLost/Recreate` 四路径在 `software/hardware` 下各跑一次探针；确认 `PYARX_SURFACE=1` 失败可干净回退旧 GDI（`PyMfcHost.cpp:55-60`）。
2. **纯装饰节点先行**：白板面板（#2）、阴影底板（#3）、静态分割线（#5 静态部分）→ 这些无输入，视觉/配色映射最直接；先把 `XamlAppearance/surfaceColorAt/m_surfaceOrder` 落到节点层。
3. **标签 + 按钮节点**（#1、#6）：节点命中派发点击；先保 hover/pressed 视觉与现有 Python 回调不回归；`BM_GETSTATE` 状态读取（`PyMfcHost.cpp:441-448`）由节点状态替代。
4. **画布节点**（#4）：把 `PlanCanvasState` 平移缩放状态机从 HWND 消息搬进节点命中；小地图/详情浮层随画布节点层处理；这一步同时验证 ② 闪是否自然消失（判据见 4.1）。
5. **覆盖层清单冻结**：Edit、ComboBox 列表、旧 ListView/Tree/Tab/Date/Link/RichEdit 保留 HWND（#9-#11）；复选框（#7）链路核清后归类。
6. **表格节点化留 S3**（#8）：数据/命中/滚动/筛选一起迁；滚动条拖拽缺口（5.2 缺口 A）在节点命中模型里补；原位编辑输入框留覆盖层。

### 6.4 必须先在 S2 验证的判据：flip-model 交换链与原生子 HWND 共存

> 风险点：`FLIP_DISCARD + BufferCount=2`（`XamlSurface.cpp:263,258`）与子 HWND 合成方式不同于旧 `WS_CLIPCHILDREN` GDI 组合；子 HWND 是独立窗口层，交换链表面在父窗客户区上，透明度/层级/重绘仲裁都需实测。

**开工前先跑通的共存判据（每条都要有可观测结果，不靠推断）：**

1. **可见性**：`PYARX_SURFACE=1` 下，覆盖层 Edit/ComboBox/旧表正常显示，不被表面 Clear 黑掉；表面 `AlphaMode=IGNORE`（`XamlSurface.cpp:266`）下子窗区域不透出后台黑。
2. **层级仲裁**：子 HWND 始终在表面之上（与现注释一致 `PyMfcProcs.cpp:1079-1080`）；子窗遮挡关系仍由 `WS_CLIPSIBLINGS`/z 序决定；用 `_diag_rgn.py` 同款 `WindowFromPoint/RealChildWindowFromPoint` 流程复核命中落点。
3. **重绘隔离**：表面 `BeginFrame Clear` 不清子 HWND 内容；子窗 `WM_PAINT` 不回写到表面后台 buffer；拖动分割条/改变尺寸时表面 `ResizeBuffers`（`XamlSurface.cpp:106-114` 区段）与子窗跟随不错位。
4. **擦底/闪烁**：表面路径 `OnEraseBkgnd` 不触发子窗背景重铺；覆盖层自身擦底行为与旧路径一致（Edit 等不闪）。
5. **设备丢失**：`SimulateDeviceLost`（`XamlSurface.h:73`；探针入口 `PyMfcProcs.cpp:1135-1137`）后表面重建，子 HWND 不丢、不叠黑块。
6. **输入链路**：子 HWND 键盘/IME/光标/下拉列表全链路可用；节点命中坐标与覆盖层客户区坐标换算正确（`ScreenToClient/ClientToScreen` 现状画布侧 `PyMfcProcs.cpp:2184`）。
7. **性能红线不回退**：拖分割条 ≤60 ms/步（现状约 34.4 ms，`_perf_newtable.txt:12-13,37,65`）；详情 ≤40 ms；平移/滚动不闪；118 项回归绿（`_perf_newtable.txt:43,72`）。

### 6.5 风险与待实测清单汇总

- 复选框（#7）是否真走 owner-draw 绘制派发：源码清单不含 `BUTTON`，链路待实测。
- 拖动期间 `WM_ERASEBKGND` 实际触发频次：影响 ② 归因权重，探针待加（本轮只读未加）。
- 表格滚动闪是否来自父窗连带失效 / GDI 每帧对象重建：待实测；新表双缓冲证据已具备。
- ⑤现场「无滚动条」的真实观感（拇指不出 vs 拖不动）：待实测；水平滚动是否被 7 列宽度触发：待实测。
- 两套 fit 规则漂移（C++ 封顶 1.0/钳 ≥0；Python 上限 4.0/未钳 ≥0）是 ④ 的代码级根因；复现模型尺寸条件待实测。
- 表面与子 HWND 层级无显式仲裁代码，仅注释说明，共存判据需 S2 实测补齐。
