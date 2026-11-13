# MFCXaml 渲染重构 S3：施工规格（节点化详情卡 + ④fit 补丁 + ⑤滚动条缺口）

> 口径：本轮**只读**，未编译、未启动 GUI、未杀进程、未改任何源文件/配置；`mfcxaml-s2-migration-map.md` 已冻结，本文不回改。
> 每条结论带 `文件:行号`；算术给算式；证据不足写「待实测」。本文只给规格与判定判据，**不含实现代码片段**。
> 源码基准：`G:\PyArx\PyMfcXaml\*`、`G:\PyArx\MfcXamlCore\*`、`G:\PyArx\tests\mfc_xaml\pp_app.py`；性能证据：`G:\PyArx\docs\_perf_newtable.txt`。

---

## 0. 成本账本现状（详情弹窗，62.97 ms 口径）

### 0.1 已实测数字（`_perf_newtable.txt`）

| 指标 | 数值 | 证据 |
|---|---|---|
| 口径 B：真实点击到探点变白卡，5 次 | 63.17 / 78.31 / 62.81 / 63.44 / 62.91 ms；中位 **63.17 ms**，均值 66.13 ms；未达 ≤40 ms 线 | `_perf_newtable.txt:146` |
| 20 个可见详情 HWND 强制 `InvalidateRect+UpdateWindow` 同步重绘合计中位 | **约 30.64 ms**（5 次中位，探针 `_probe_detail_hwnd_paint.py:35-41`） | `_perf_newtable.txt:128`；探针枚举 `_probe_detail_hwnd_paint.py:19-26` |
| `DT_PANEL` 单控件同步重绘中位 | 19.11 ms（min 15.56 / max 19.45） | `_perf_newtable.txt:107,128` |
| 底板阴影静态 DIB 缓存命中 | `[shadow-cache] HIT id=1220 272x346 blit=0.02~0.04 ms`；本体绘制已命中缓存，探针里的高值主要是 `BeginPaint/UpdateWindow` 外围开销 | `_perf_newtable.txt:148`；缓存代码 `PyMfcProcs.cpp:1179-1208,1283-1317` |
| 阶段拆分：控件赋值 / showDetail+_layout / pump(30) | 3.43 / 8.63 / 34.48 ms；非泵 `fill_detail` 3.19 ms；`pump(30)` 28.58 ms；空 pump 30.26 ms | `_perf_newtable.txt:129` |
| 归因（现有证据） | 剩余成本在「21 个 HWND 的 `BeginPaint` / 外围分发 / OS 消息队列与合成」，不支持继续压 DT_PANEL 本体 | `_perf_newtable.txt:151,225` |

### 0.2 「21 个 HWND」与源码枚举 20 个的口径差（待实测）

- 探针 `_probe_detail_hwnd_paint.py:19-26` 的 `names` 字典只枚举 **20 个** id；`_perf_newtable.txt:14,151,225` 口径写「21 个」。
- 差额 1 个候选：ComboBox 展开时的系统下拉列表子窗（`addCombo` 带 `WS_VSCROLL | CBS_DROPDOWNLIST`，`PyMfcHost.cpp:951-955`；列表是系统独立子窗口）、或表格筛选临时 Edit（`Table.h:138 m_hFilterEdit`，非详情卡）。**本轮只读未展开枚举，差额归属待实测**；下表按探针 20 个可见详情控件逐个列。

---

## 1. 详情浮层控件逐个台账（20 个可见 HWND）

### 1.1 台账字段说明

- **创建入口**：`pp_app.py` 调用点 → `PyMfcHost.cpp` 建窗函数与窗口类/样式。
- **当前绘制路径**：
  - 路径 A「owner-draw 经父 `WM_DRAWITEM`」：`STATIC SS_OWNERDRAW` / `BUTTON BS_OWNERDRAW` → `PyMfcOwnerDrawProc` `WM_PAINT` 自造 `DRAWITEMSTRUCT` 发父窗（`PyMfcHost.cpp:427-456`）→ 父 `OnDrawItem`（`PyMfcProcs.cpp:1756-1828`）。
  - 路径 B「皮肤链 `paintSkinControl` 离屏 DIB + BitBlt」：控件被 `PyMfcSkinWindowProc` 接管（`PyMfcProcs.cpp:1499-1521`）→ `paintSkinControl`（`PyMfcProcs.cpp:1092-1281`）→ 按类/skinKind 派发。
  - 路径 C「控件自身 `WM_PAINT`」：本表无（新表格 `PyMfcTable` 才是，`Table.cpp:625-821`，不在详情卡）。
- **节点化原语**：`XamlCanvas` 接口（`XamlDraw.h:17-130`）；皮肤组合件 `PyMfcSkin.h`。

### 1.2 判定总表

| # | id | 名称 | 创建入口（pp_app.py → Host.cpp） | 类 / 样式 | 当前路径 | 现有绘制函数与文件:行号 | 节点化原语 | 判定 |
|---:|---:|---|---|---|---|---|---|---|
| 1 | 1220 | `DT_PANEL` 详情卡底板+阴影 | `pp_app.py:965-966` → `addShadowBox` `PyMfcHost.cpp:1339-1365`；皮肤件 `addSkinBox` `PyMfcHost.cpp:1323-1337` | 注册类 `kSkinBoxClass`，`kSkinBoxShadow`（`PyMfcHost.h:51-53` 区段） | B | `paintSkinControl` 阴影分支 `PyMfcProcs.cpp:508-513` → `pymfcskin::drawShadowBox` `PyMfcSkin.h:1045-1071`；离屏 DIB/BitBlt `PyMfcProcs.cpp:1210-1232,1280-1281` | `drawShadowBox(cv, rect, ShadowBoxStyle)` 复用同函数（`PyMfcSkin.h:1045`）；内部 12 层 `fillRoundRect` 扩散 + `fillRoundRect` 底板 + `strokeRoundRect` 描边 | **转节点**（画布 D2D 层阴影底板节点，z 最低层浮层） |
| 2 | 1221 | `DT_TITLE` 标题「节点」 | `pp_app.py:971` → `addLabel` `PyMfcHost.cpp:905-908` | `STATIC` `SS_OWNERDRAW` | A | `OnDrawItem` 标签/底板分支：无样式时 `fillRect(surface)` `PyMfcProcs.cpp:1895`；`fillRoundRect` 仅按钮 `PyMfcProcs.cpp:1900`；文字 `cv->text(...)` `PyMfcProcs.cpp:1997`；字号/字重来自 `_font_ids`（`pp_app.py:973`） | `xamlDrawText` 等价：`XamlCanvas::text(rect, wstring, fg, align, ptSize, face)`（`XamlDraw.h:46-47`；D2D 实现 `XamlD2D.h:388-429`）；标题粗体走 `setTextStyle(weight=bold)`（`XamlDraw.h:125-129`；`XamlD2D.h:364-379`） | **转节点**（纯展示文字节点） |
| 3 | 1222 | `DT_TIP` 提示文字（空态） | `pp_app.py:1016-1018` → `addLabel` | `STATIC` `SS_OWNERDRAW` | A | 同上，`text` `PyMfcProcs.cpp:1997` | `XamlCanvas::text`（`XamlDraw.h:46`） | **转节点** |
| 4 | 1225 | `DT_L0(编号)` 标签 | `pp_app.py:978`（i=0）→ `addLabel` | `STATIC` `SS_OWNERDRAW` | A | `OnDrawItem` 标签分支 `PyMfcProcs.cpp:1895,1997` | `XamlCanvas::text` | **转节点** |
| 5 | 1226 | `DT_L0(名称)` 标签 | `pp_app.py:978`（i=1） | `STATIC` `SS_OWNERDRAW` | A | 同上 | `XamlCanvas::text` | **转节点** |
| 6 | 1227 | `DT_L0(上游)` 标签 | `pp_app.py:978`（i=2） | `STATIC` `SS_OWNERDRAW` | A | 同上 | `XamlCanvas::text` | **转节点** |
| 7 | 1228 | `DT_L0(电缆型号)` 标签 | `pp_app.py:978`（i=3） | `STATIC` `SS_OWNERDRAW` | A | 同上 | `XamlCanvas::text` | **转节点** |
| 8 | 1229 | `DT_LV`「电压」标签 | `pp_app.py:987-988` → `addLabel` | `STATIC` `SS_OWNERDRAW` | A | `OnDrawItem` 标签分支 | `XamlCanvas::text` | **转节点** |
| 9 | 1230 | `DT_LL`「长度 m」标签 | `pp_app.py:991-992` | `STATIC` `SS_OWNERDRAW` | A | 同上 | `XamlCanvas::text` | **转节点** |
| 10 | 1231 | `DT_LLOAD`「负载 kW」标签 | `pp_app.py:995-996` | `STATIC` `SS_OWNERDRAW` | A | 同上 | `XamlCanvas::text` | **转节点** |
| 11 | 1240 | `D_CODE` 编号输入框 | `pp_app.py:980`（i=0，`D_F0+0`，常量 `pp_app.py:84`）→ `addEdit` `PyMfcHost.cpp:910-913` | `EDIT`，`WS_BORDER | ES_AUTOHSCROLL` | B | 皮肤链 Edit 分支 `PyMfcProcs.cpp:889-938` → `pymfcskin::drawEdit` `PyMfcSkin.h:506-515`；选区/光标状态 `caretBlinkStates` `PyMfcProcs.cpp:145-155`；输入事件 `EN_CHANGE/EN_KILLFOCUS` `PyMfcProcs.cpp:15-16,129-136` | **不转节点**：保留 HWND 输入覆盖层；其视觉（边框/文字/光标）后续可由表面节点叠加，caret/IME/焦点仍归 HWND | **保留 HWND**（必须：键盘输入、光标、IME、选区） |
| 12 | 1241 | `D_NAME` 名称输入框 | `pp_app.py:980`（i=1） | `EDIT` | B | 同 #11 | 同 #11 | **保留 HWND** |
| 13 | 1242 | `D_UP` 上游输入框 | `pp_app.py:980`（i=2） | `EDIT` | B | 同 #11 | 同 #11 | **保留 HWND** |
| 14 | 1243 | `D_CABLE` 电缆型号输入框 | `pp_app.py:980`（i=3） | `EDIT` | B | 同 #11 | 同 #11 | **保留 HWND** |
| 15 | 1244 | `D_VOL` 电压下拉 | `pp_app.py:989-990` → `addCombo` `PyMfcHost.cpp:951-955` | `COMBOBOX` `CBS_DROPDOWNLIST | WS_VSCROLL` | B | 皮肤链 Combo 分支 `PyMfcProcs.cpp:624-649` → `pymfcskin::drawCombo` `PyMfcSkin.h:150-159`（`fillRect` + `fillRoundRect` 3px + 文字）；下拉列表为系统子窗 | **保留 HWND**：选择下拉列表是系统独立子窗，不能由画布节点代劳；外观可后续由节点叠加 | **保留 HWND**（必须：下拉列表、项选择交互） |
| 16 | 1245 | `D_LEN` 长度输入框 | `pp_app.py:993-994` → `addEdit` | `EDIT` | B | 同 #11 | 同 #11 | **保留 HWND** |
| 17 | 1246 | `D_LOAD` 负载输入框 | `pp_app.py:997-998` → `addEdit` | `EDIT` | B | 同 #11 | 同 #11 | **保留 HWND** |
| 18 | 1250 | `D_SAVE`「保存并重绘」按钮 | `pp_app.py:1008-1009` → `addButton` `PyMfcHost.cpp:900-903`；回调 `onDetailSave` `pp_app.py:1021` | `BUTTON` `BS_OWNERDRAW` | A | 按压态 `BM_GETSTATE` `PyMfcHost.cpp:441-448`；`OnDrawItem` 按钮底板 `fillRoundRect(...,radius 8)` `PyMfcProcs.cpp:1897-1900`；文字 `PyMfcProcs.cpp:1997`；回调经 `BN_CLICKED` | 视觉可转节点：`xamlDrawBackplate`/`fillRoundRect` + `XamlCanvas::text`；点击需节点命中测试（等价当前 `BN_CLICKED` 派发） | **S3 先保留 HWND**（输入/点击/焦点过渡方案）；S3 验收后可随 S2 节点命中模型再转视觉节点 |
| 19 | 1251 | `D_ADDCHILD`「＋下游节点」按钮 | `pp_app.py:1010-1011`；回调 `onDetailAddchild` `pp_app.py:1022` | `BUTTON` `BS_OWNERDRAW` | A | 同 #18（探针耗时第 2，0.90 ms，`_perf_newtable.txt:108`） | 同 #18 | **S3 先保留 HWND** |
| 20 | 1252 | `D_DEL`「删除」按钮 | `pp_app.py:1012-1013`；回调 `onDetailDel` `pp_app.py:1023` | `BUTTON` `BS_OWNERDRAW` | A | 同 #18 | 同 #18 | **S3 先保留 HWND** |

### 1.3 判定计数（20 个可见控件）

| 类别 | 数量 | id | 说明 |
|---|---:|---|---|
| 判 **转节点**（纯展示/装饰，无输入） | **10** | 1220 底板；1221/1222/1225-1231 共 9 个标签 | 底板 1 + 标签 9 |
| 判 **保留 HWND**（输入/下拉/点击） | **10** | 1240-1243、1245、1246 共 6 个 Edit；1244 Combo；1250-1252 共 3 个 Button | Edit 6 + Combo 1 + Button 3 |
| 后续可二次转节点 | 3 | 1250/1251/1252 按钮视觉 | 待节点命中测试落地后；本轮不拆 |

> 备注：若按「按钮视觉也先行并入画布、点击暂用覆盖层透明热区」的激进口径，转节点为 13、保留为 7（仅 6 Edit + 1 Combo）；该口径需先验证透明热区与原 `BN_CLICKED` 时序一致，**列为可选路线，不作 S3 默认**。

---

## 2. 详情卡底板 + 阴影并入画布 D2D 层规格

### 2.1 `ShadowBoxStyle` 当前取值（DT_PANEL 实例）

调用点 `pp_app.py:965-966` 传参：`pad=14, radius=6, dx=0, dy=4, blur=12, shadow='#1A101828', bg='#FFFFFF', back='#FFFFFF', border='#D0D5DD', borderW=1`。

| 字段 | 取值 | 规格来源 | 结构默认值（对照） |
|---|---|---|---|
| `pad` | 14 | `pp_app.py:966`；映射 `PyMfcHost.cpp:1346,1365`（HWND 外扩 `w+2*pad`） | 12，`PyMfcSkin.h:1023` |
| `radius` | 6 | `pp_app.py:966`；`PyMfcHost.cpp:1347` | 6，`PyMfcSkin.h:1024` |
| `dx, dy` | 0, 4 | `pp_app.py:966`；`PyMfcHost.cpp:1348-1349` | 0, 4，`PyMfcSkin.h:1025` |
| `blur` | 12 | `pp_app.py:966`；`PyMfcHost.cpp:1350` | 12，`PyMfcSkin.h:1026` |
| `alpha` | 0x1A = **26**（来自 `#1A101828` 首字节） | `PyMfcHost.cpp:1352-1358` | 26，`PyMfcSkin.h:1027` |
| `shadow` | RGB(16,24,40) | 同上；`PyMfcSkin.h:1028` 默认同值 | RGB(16,24,40) |
| `bg` | 白 RGB(255,255,255) | `pp_app.py:966`；`PyMfcHost.cpp:1360` | 白，`PyMfcSkin.h:1029` |
| `back` | 白 | `PyMfcHost.cpp:1361` | 白，`PyMfcSkin.h:1030` |
| `border` | #D0D5DD = RGB(208,213,221) | `PyMfcHost.cpp:1362` | 同值，`PyMfcSkin.h:1031` |
| `borderW` | 1 | `PyMfcHost.cpp:1351` | 1，`PyMfcSkin.h:1032` |

### 2.2 阴影层数、半径与现有绘制次序（`drawShadowBox`，`PyMfcSkin.h:1045-1071`）

1. 可见矩形 `vis = win 内缩 pad`（`PyMfcSkin.h:1047`）。
2. `fillRect(win, bg)` 先铺满整个外扩窗（`PyMfcSkin.h:1052`）——注释说明：pad 区不铺会留黑（`PyMfcSkin.h:1050-1051`）。
3. 阴影扩散：`blur=12` → **12 层**循环 `i=12..1`（`PyMfcSkin.h:1056`）；每层 `k=blur-i+1`，`alpha_i = alpha*k²/blur²`（`PyMfcSkin.h:1058-1059`）；矩形 `OffsetRect(dx,dy) + InflateRect(i,i)`，颜色 `mixColor(shadow,bg,a)`；`fillRoundRect(r, c, c, r0+i)`（`PyMfcSkin.h:1061-1065`），即每层圆角半径 `r0+i`。
4. 底板 `fillRoundRect(vis, back, back, r0)`（`PyMfcSkin.h:1068`）。
5. 描边 `strokeRoundRect(vis, border, r0, borderW)`（`PyMfcSkin.h:1070`）。

### 2.3 并入表面层后的节点次序与 z 值（规格，沿用现有视觉层级）

| 层序（自底向上） | 节点内容 | 绘制原语 | z 规则 |
|---:|---|---|---|
| 1 | 画布页面底色 / 已有 plan 内容 | 现有 `drawPlanCanvas` 路径（`PyMfcHost.cpp:3821-3911`） | 基底层 |
| 2 | 阴影 12 层扩散（0,4 偏移、外扩 i、半径 6+i、预混 alpha） | 复用 `drawShadowBox` 内循环等价 `fillRoundRect`（`XamlD2D.h:248-280`） | 浮层底板阴影组，z 高于 plan、低于底板实色 |
| 3 | 底板实色圆角矩形 + 1px 描边 | `fillRoundRect`（`XamlD2D.h:248-280`）+ `strokeRoundRect`（`XamlD2D.h:319-331`） | 浮层底板节点 |
| 4 | 卡内标签文字（标题/字段标签/提示，#2-#10） | `XamlCanvas::text`（`XamlDraw.h:46`；`XamlD2D.h:388-429`） | 底板之上 |
| 5 | 保留 HWND 覆盖层：6 Edit + 1 Combo + 3 Button（#11-#20） | 各自现有 HWND 链 | 最上层覆盖；坐标沿用 `_detail_rc`（`pp_app.py:962` 记录） |

- 节点矩形：外扩阴影区按 `pad=14` 计算（对应当前 HWND `x-pad,y-pad,w+2pad,h+2pad`，`PyMfcHost.cpp:1365`）；可见卡体矩形 = 内缩 14。
- 变换：详情卡坐标是父客户区固定偏移（`DT_X/DT_Y/DT_W/DT_H`，`pp_app.py:45,965,967`），不随画布 `zoom/ox/oy` 变换；节点注册时父变换记为恒等（对照 S2 地图 §3.2 父坐标变换字段）。
- 缓存：现有静态阴影 DIB 缓存（`PyMfcProcs.cpp:1195-1204,1283-1316`）在节点化后改为表面帧内的静态底板节点；阴影 12 层是否仍走预混静态缓存，由 S3 实测 D2D 层耗时后定，**不在本轮下结论**。

### 2.4 为什么这样做能把多次 `BeginPaint` 合成降到一次 `Present`（可量化判据）

**现状机制**：20 个 HWND 各自 `BeginPaint/EndPaint`（owner-draw 链 `PyMfcHost.cpp:430`；皮肤链 `PyMfcProcs.cpp:1502`；表格链 `Table.cpp:720` 对照），OS 按窗口分别合成；探针证据：20 窗同步重绘 30.64 ms（`_perf_newtable.txt:128`），其中底板 19.11 ms 主要是外围 BeginPaint/UpdateWindow 开销而非 D2D 本体（`_perf_newtable.txt:148`）。

**节点化后机制**：底板 + 9 个标签并入表面同一帧 `BeginFrame→D2D 绘制→EndFrame→Present`（`XamlSurface.h:40,44,48`；`XamlSurface.cpp:66-104`），不再为装饰件开子窗。

**S3 验收量化判据（改前/改后同口径采集）：**

| 指标 | 现状基线（证据） | S3 目标判据 |
|---|---|---|
| 单帧 `BeginPaint` 次数（详情卡可见期间，一次点击重绘） | 约 20 次子窗 + 宿主；探针逐 HWND `InvalidateRect+UpdateWindow`（`_probe_detail_hwnd_paint.py:37,39`） | 保留 HWND（6 Edit + 1 Combo + 3 Button = 10，若按钮转视觉节点则 7）之外，装饰件 `BeginPaint` 次数 = 0；表面帧 `BeginFrame` 1 次 |
| `Present` 次数 | 旧 GDI 路径无显式交换链 Present；子窗各自提交 | 表面帧 **1 次 Present**（`XamlSurface.cpp:101-102`）；装饰件不触发额外合成 |
| 详情点击到变白卡（口径 B） | 中位 63.17 ms（`_perf_newtable.txt:146`） | **≤40 ms**；其中节点装饰绘制应接近底板缓存命中 blit 量级（现 0.02~0.04 ms，`_perf_newtable.txt:148`），剩余主要在保留 HWND 的输入窗合成 |
| 20 窗同步重绘合计 | 30.64 ms（`_perf_newtable.txt:128`） | 装饰件 10 个不再进该统计；保留 HWND 逐个耗时与现状对比不回退 |

> 不确定点：表面 `PYARX_SURFACE=1` 与 10 个保留子 HWND 的合成层级当前仅注释说明（`PyMfcProcs.cpp:1079-1080`），无显式仲裁代码；共存判据沿用 S2 地图 §6.4，S3 开工前先过该 7 条。

---

## 3. ④ 初始视图未 fit：精确补丁规格

### 3.1 三处改点（规格，不给代码片段）

**A. `fitView()` — `pp_app.py:1180-1190`**

| 行号 | 现状 | 应对齐的 C++ 口径（`PyMfcHost.cpp:3453-3472`） |
|---|---|---|
| `pp_app.py:1185` | `k = min(aw/W, ah/H) * 0.92`（0.92 padding） | C++ 用客户区减 8px 做 padding：`kw=(rc.right-8)/w`、`kh=(rc.bottom-8)/h`（`PyMfcHost.cpp:3464-3465`）。规格：padding 口径二选一——要么与 C++ 统一为减 8px，要么保留 0.92 但在下限/上限上对齐；**两套 padding 不得继续漂移**。 |
| `pp_app.py:1186` | `k = max(0.2, min(4.0, k))`，**上限 4.0** | C++ `zoom = min(min(kw,kh), 1.0f)` 封顶 **1.0**，下限 0.2（`PyMfcHost.cpp:3466-3467`）。规格：Python 上限改 1.0，保留 0.2 下限。 |
| `pp_app.py:1187-1188` | `ox=int((aw-W*k)/2)`、`oy=int((ah-H*k)/2)`，**未钳 ≥0** | C++ 居中后 `if(ox<0) ox=0; if(oy<0) oy=0`（`PyMfcHost.cpp:3468-3471`）。规格：`ox/oy` 钳 `≥0`。 |

**B. `centerKeep()` — `pp_app.py:1216-1225`**

| 行号 | 现状 | 规格 |
|---|---|---|
| `pp_app.py:1220` | `zoom = getPlanView(CV)[0] or 1.0` | 沿用；不引入 `k>1` 上限问题（zoom 来自用户缩放，保留 [0.2,4] 手动缩放区间，与 `zoomAt`/`setZoom` 一致，`pp_app.py:1196,1208`）。 |
| `pp_app.py:1223-1224` | `ox/oy` 居中，**未钳 ≥0** | 同 A 行：钳 `≥0`，对齐 `PyMfcHost.cpp:3470-3471`。 |

**C. `rebuild()` — `pp_app.py:1136-1143`**

| 行号 | 现状 | 规格 |
|---|---|---|
| `pp_app.py:1136-1137` | `loadPowerScene(CV/MINI)`；C++ 装场景内部已调 `fitPlanCanvas`（`PyMfcHost.cpp:3534`；`setPowerCanvasPlan` `PyMfcHost.cpp:3511`；建窗后 `PyMfcHost.cpp:3484-3486`） | 现状 C++ fit 先落一遍，随后被 Python `fitView` 覆盖（`pp_app.py:1140-1141`）。规格二选一：① 改 A/B 后保留 Python 覆盖，仅做钳制对齐；② 移除 Python `fitView` 覆盖，统一走 C++ `fitPlanCanvas` 口径（更稳，避免双规则漂移）。`centerKeep` 分支（`pp_app.py:1142-1143`）保留。 |
| `pp_app.py:1138-1139` | `setPlanOptions` | 与 fit 无关，不动。 |

### 3.2 `viewArea()` 视口宽度口径（`pp_app.py:1175-1178`）

- 现状：`used = (sideWidth()+SPLIT_W) if editing else 0`；返回 `max(200, CV_W-used), CV_H`（`pp_app.py:1177-1178`）。
- 两种状态应使用的视口宽度：
  - **编辑器关闭**（`editing=False`）：`used=0`，视口宽 = 画布实际客户区宽；常量 `CV_W = CW-16`（`pp_app.py:42`）。
  - **编辑器打开**（`editing=True`）：`used=sideWidth()+SPLIT_W`；画布实际宽由分割条拖动维护在 `self.cvW`（`pp_app.py:315,505-507`：`cvW = max(CV_MIN_W, cw-CV_X-8-ed_w)`）。
- **漂移点**：`viewArea` 用常量 `CV_W-used`，未读 `self.cvW`；分割条拖动后 `self.cvW` 已更新（`pp_app.py:507`），fit/居中基准可能与真实客户区不一致。规格：`viewArea` 宽度以**画布实际客户区**为准（优先 `GetClientRect(CV)` 对应 C++ `PyMfcHost.cpp:3460-3461`；Python 侧可用 `self.cvW/self.cvH` 对齐），`CV_W` 仅作初始常量；`sideWidth()` 口径不变（`pp_app.py:491-495`）。
- 算术校验（默认常量）：`CW=APP_W-16`、`CV_W=CW-16`（`pp_app.py:36,42`）；`SPLIT_W=6`（`pp_app.py:55`）；`CV_MIN_W=320`（`pp_app.py:59`）。

### 3.3 改前/改后与 C++ 一致性判定方法（可对比 `getPowerCanvasView`）

1. 启动后 `rebuild(fit=True)`，读取 `getPlanView(CV)`（Python 包装，`pp_app.py:1193,1205,1220`；底层 `PyMfcHost.cpp:3607-3616`）返回 `(zoom, ox, oy)`。
2. 独立调 C++ `fitPlanCanvas` 口径复算：用 `GetClientRect(CV)` 宽高、模型 `width/height`（`self.view['width'/'height']`，`pp_app.py:1183`），按 `PyMfcHost.cpp:3464-3471` 算 `zoom_c/ox_c/oy_c`。
3. 判定：改后 `zoom ≤ 1.0`、`ox ≥ 0`、`oy ≥ 0`，且与 C++ 复算值一致（padding 口径选定后误差仅取整级别）；改前预期可见 `zoom>1` 或 `ox<0`（对应「整图放大、左侧被裁」）。
4. 复测场景：编辑器开/关各一次 fit；分割条拖动后再 fit；小地图 `MINI` 跟随不参与该判定（`setPlanOptions(MINI,...,CV)` `pp_app.py:1138,1202,1214`）。

---

## 4. ⑤ 表格滚动条真实缺口判定

### 4.1 现状事实（源码证据）

- 垂直自绘 track + thumb：`Table.cpp:798-814`（`sbW=10`；track `FillRect` L801-803；`MaxScroll>0` 时 `thumbH/thumbY` L804-809、thumb `FillRect` L810-813）。
- 滚轮：`WM_MOUSEWHEEL` → `ScrollBy(±1)`（`Table.cpp:705-709`）；键盘/选中跟随 `EnsureVisible`（`Table.cpp:591-601`；宿主 `tableEnsureVisible` `PyMfcHost.cpp:1178`）。
- 左键命中分支只判表头（`Table.cpp:639-650`）与 `Hit` 行/格（`Table.cpp:651-656`）；**无 track/thumb 命中分支、无鼠标捕获、无 `WM_VSCROLL` 处理**；注册样式 `Table.cpp:228` 无 `WS_VSCROLL/WS_HSCROLL`。

### 4.2 两种情形的可观测判据

| 情形 | 定义/触发 | 可观测判据（不改码前先采集） |
|---|---|---|
| **情形 A：拇指不出（`MaxScroll==0`，行数本来不够滚）** | `MaxScroll = all>vis ? all-vis : 0`（`Table.cpp:539-544`；`vis=VisibleCount` `Table.cpp:532-537`；`all=ViewRows`） | ① 读数 `MaxScroll()==0`（接口 `Table.h:93`）；② `Table.cpp:804` 条件不成立，无 thumb 矩形；③ 滚轮 `ScrollBy` 被 `m_scroll>mx` 钳回（`Table.cpp:584-587`）；④ 行数 `ViewRows() ≤ VisibleCount()`。属**正常不滚**，非缺陷。 |
| **情形 B：拇指存在但拖不动** | `MaxScroll>0`，thumb 已画（`Table.cpp:808-813`），但鼠标按在 track/thumb 上无响应 | ① 读数 `MaxScroll()>0`；② 截图/探针确认 thumb 像素存在；③ 左键落在 thumb/track 矩形内（`Table.cpp:798-811` 几何）时，`WndProc` 不进入任何拖拽分支；④ 拖动期间 `m_scroll` 不变、无 `Invalidate`；⑤ 滚轮仍可滚（区分交互缺失而非滚动计算错）。 |

> 待实测：现场用户报「无滚动条」属 A 还是 B；`_probe_scroll80.py` 只覆盖垂直滚动生效（`_perf_newtable.txt:39-40`），未覆盖拇指拖拽与空态拇指。

### 4.3 若补拖拽（情形 B），规格

- **`Table.h` 需新增成员**（规格字段，不写代码）：拖拽状态（`dragScroll` 标志、起始 `m_scroll`、按下时 thumb 几何/起始鼠标 y）；复用现有 `m_scroll`（`Table.h:133`）、`VisibleCount/MaxScroll/ScrollPos`（`Table.h:91-95`）。
- **`Table.cpp WndProc` 左键分支**（`Table.cpp:635-656` 区段）：
  1. 先于表头/行命中，按 `Table.cpp:798-811` 的 track/thumb 几何做命中区分：点 thumb 走 thumb 拖拽；点 track 空白走翻页（规格：一屏一行可视数，或沿用现有翻页语义，**翻页步长待实测对齐 HTML 原型**）。
  2. 命中后 `SetCapture`（对照画布拖拽 `PyMfcProcs.cpp:2214` 的捕获模式）；`WM_MOUSEMOVE` 按拇指位移反算 `m_scroll`，钳制区间 `[0, MaxScroll()]`（与 `ScrollBy` 一致，`Table.cpp:584-587`）；`WM_LBUTTONUP` 释放捕获。
  3. 滚动状态与 `ScrollBy/ScrollPos/MaxScroll` 同源：只改 `m_scroll`，不另立偏移；重绘 `Invalidate` 沿用 `Table.cpp:588`。
- **事件/回调**：现有 `TableEventFn`（`Table.h:59,150`）是否需要新增滚动事件，取决于 Python 侧是否跟随选中行；本轮规格为**不新增回调**，纯内部拖拽。

### 4.4 水平滚动是否真需要（算术）

- 新表接收列宽：`w.setTableColumns(TBL, COLW[:7], COLS[:7])`（`pp_app.py:1057`）；默认 `COLW=[78,100,70,130,52,100,56,26]`（`pp_app.py:103`），取前 7。
- 原始求和（前 7）：`78+100+70+130+52+100+56 = 586`。
- 表格客户宽：`addTable` 宽 `ED_W - 2*ED_PAD = 640 - 2*10 = 620`（`pp_app.py:53,1056`；`ED_PAD=10` `pp_app.py:60`）；垂直条占 `sbW=10`（`Table.cpp:798`），内容宽 `620-10 = 610`。
- 余量：`610 - 586 = 24 px > 0` → **默认宽度下无水平溢出，不需水平滚动**。
- 例外与待实测：
  - 引擎 `rules.table.widths` 可覆盖 `COLW`（`pp_app.py:103` 取 `ENG.rules.table.get('widths')`）；覆盖值之和 >610 时才溢出，**实际引擎值待实测**。
  - 编辑器最小宽 `SPLIT_MIN_W=360`（`pp_app.py:57`）：内宽 `360-20=340`、内容 `330 < 586`；此时列宽若不缩放会横向溢出。旧 ListView 路径按 `gridColumns` 比例缩放到 `avail`（`pp_app.py:607-614`），新表 `SetColumns` 直接存原始宽（`Table.cpp:509-515`）不缩放——**窄侧板下是否横向裁切，待实测**；若发生，优先做列宽比例缩放而非先上水平滚动条。

---

## 5. S3 施工顺序、对照回退点与硬线

### 5.1 建议顺序（每步可独立回退）

| 步 | 内容 | 前置 | 对照/回退点 |
|---:|---|---|---|
| S3-0 | 共存复核：`PYARX_SURFACE=1` 下 10 个保留子 HWND 可见、层级、擦底、设备丢失（S2 地图 §6.4 七条） | S1 表面已在 | 失败即回 `PYARX_SURFACE=0` 旧 GDI 链；不改控件结构 |
| S3-1 | ④ fit 口径对齐（`fitView`/`centerKeep` 钳制 + `viewArea` 实际客户区；§3） | 无 | 纯 Python 层改点；回退 = 还原 `pp_app.py:1186,1187-1188,1223-1224,1177-1178`；用 §3.3 判据对比 `getPlanView` |
| S3-2 | 底板节点化（`DT_PANEL` 并入表面 D2D 层，12 层阴影 + 底板 + 描边，§2.3 层序 2-3） | S3-0 | 开关级回退：底板节点关闭后 `DT_PANEL` 回到 `kSkinBoxShadow` HWND；保留 6 Edit + 1 Combo 不动 |
| S3-3 | 9 个标签节点化（层序 4，纯 `text` 节点） | S3-2 | 逐类回退到 `SS_OWNERDRAW` HWND；标签无输入，风险低 |
| S3-4 | 情形 B 拖拽补齐（如实测确认；§4.3） | S3-1 无关 | 独立于表面层；只动 `Table.h/Table.cpp`；回退 = 移除拖拽分支，滚轮/`EnsureVisible` 不受影响 |
| S3-5 | 按钮视觉是否二次节点化（可选，§1.3 激进口径） | S3-3 + 节点命中测试 | 不默认做；保留 HWND 到后续 S2 命中模型落地 |

### 5.2 开关对照矩阵（沿用 S2 地图 §6.2）

- `PYARX_SURFACE=0`：全程旧 GDI/皮肤链，作为每步对照基线；S3 节点化路径只在 `=1` 生效。
- `PYARX_D2D_RT=software/hardware`：底板 12 层阴影与文字在两档下分别采样；软件档是 `_perf_newtable.txt` 现有基线口径（探针 `_probe_detail_hwnd_paint.py:6`）。
- 回退动作仅限配置开关/还原改点；本轮只读，不执行编译与启动。

### 5.3 S3 完成后应能宣称的硬线

| 硬线 | 现状 | S3 目标 | 证据/判据 |
|---|---|---|---|
| 详情点击到变白卡 | 中位 63.17 ms（`_perf_newtable.txt:146`） | **≤40 ms**；装饰件 `BeginPaint=0`、表面 `Present=1`（§2.4） | 同口径 B 探针；`_perf_newtable.txt:146` 采集方式复用 |
| ④ 初始视图 fit | `zoom` 可 >1、`ox/oy` 可负（`pp_app.py:1186-1188`） | `zoom≤1.0`、`ox≥0`、`oy≥0`，与 C++ `fitPlanCanvas` 复算一致（§3.3） | `getPlanView(CV)` 读数对照 |
| ⑤ 滚动条 | 拇指拖拽未接（§4.1）；空态拇指不出属正常 | 情形 B 补齐拖拽；情形 A 保持 `MaxScroll==0` 不出拇指；默认宽度无水平滚动（§4.4 算术） | §4.2 两情形判据 |
| 不回退项 | 拖分割条现状 34.4 ms/步（`_perf_newtable.txt:12-13`）；118 回归绿（`_perf_newtable.txt:136,150`） | 拖分割条 ≤60 ms/步；118 回归绿；平移/滚动不闪 | 复用 `_run_regression.ps1` 与现有探针口径 |

### 5.4 待实测清单（不在本轮下结论）

1. 「21 vs 20」差额 HWND 归属（§0.2）。
2. 底板 12 层阴影节点化后是否保留静态缓存、D2D 层耗时量级（§2.3 末）。
3. 表面交换链与保留子 HWND 层级仲裁无显式代码（§2.4）。
4. ⑤现场属空态拇指（A）还是拖拽失效（B）；引擎覆盖列宽；窄侧板列宽是否被裁切（§4.2、§4.4）。
5. 滚动条 track 翻页步长语义（§4.3）。
