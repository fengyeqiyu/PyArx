# S1 复验与回归口径纠偏（mfcxaml-s1-verify）

> 范围：只做复验 + 回归口径纠偏，**不进 S2，不改 C++ 渲染代码**。
> 环境：Windows 本机；Python `D:\Program Files\Python314\python.exe`；`PYARX_SURFACE` 缺省/`0`=旧 GDI 路径，`1`=D3D11 交换链路径；`PYARX_D2D_RT=software`。工程 `G:\PyArx` 非 git 仓库。
> 本轮未重编 C++；既有产物基线：`X64\Release\PyMfcXaml.pyd` mtime 2026-11-11 19:58:34，`MfcXamlCore.dll` mtime 2026-11-11 19:57:36。

---

## (a) `=1` 与 `=0` 一致性

### a.1 既有抓帧逐字节复算

文件：`G:\PyArx\docs\_diag_screen_on.npy` / `_diag_screen_off.npy`。

- shape：`(880, 1340, 4)`，dtype uint8（两文件一致）。
- 像素数 `880*1340 = 1,179,200`；字节数 `880*1340*4 = 4,716,800`。
- 逐像素逐通道 `a != b`：**不一致像素 0 / 1,179,200 = 0.000000%**；最大绝对通道差 0；逐字节差 0 / 4,716,800 = 0.0000%。
- 复算脚本口径：numpy 2.5.4（`G:\PyArx\pylibs`），`(a != b).any(axis=2)` 取像素，再按通道阈值 10 复核，结论相同。

### a.2 六场景同状态对照（本轮新抓帧）

探针：`G:\PyArx\tests\mfc_xaml\_s1v_scene.py`（按 `PYARX_SURFACE` 启动一次，固定窗口位置/尺寸，依次抓 6 场景整窗 DIB）；
对照：`G:\PyArx\tests\mfc_xaml\_s1v_compare.py`；原始帧：`docs\_s1v_{0,1}_{scene}.npy`；汇总：`docs\_s1v_scene_diff.txt`。

窗口两路径同状态：`rect=(100,100,1440,980)`，1340×880；`viewArea=(662,755)`；详情弹出 `visible=True code=AT1-1`。

| 场景 | 不一致像素(精确) | 精确比例 | 阈值>10 像素 | 阈值比例 | 差异集中矩形 (x1,y1,x2,y2) |
|---|---:|---:|---:|---:|---|
| 初始 initial | 305 / 1,179,200 | 0.025865% | 296 | 0.025102% | (605,872,689,879) |
| 拖窗口 winmove | 229 / 1,179,200 | 0.019420% | 206 | 0.017469% | (593,872,674,879) |
| 拖分割条 split | 229 / 1,179,200 | 0.019420% | 206 | 0.017469% | (593,872,674,879) |
| 画布平移 pan | 229 / 1,179,200 | 0.019420% | 206 | 0.017469% | (593,872,674,879) |
| 表格滚动 scroll | 229 / 1,179,200 | 0.019420% | 206 | 0.017469% | (593,872,674,879) |
| 弹详情 detail | 229 / 1,179,200 | 0.019420% | 206 | 0.017469% | (593,872,674,879) |

目标 <0.5%：**六场景全部达标**（最高 0.0259%，约为阈值的 1/19）。

差异定位：
- 全部差异压在窗口最底部 8px 横带 `y=872..879`（状态栏文字行），其余客户区与子控件像素逐通道一致。
- 带内背景两路径相同（采样 `(110,110,110)`、`(115,111,108)` 两侧一致）；差异像素是灰色文字抗锯齿灰阶（例：`on (107,107,107)` / `off (72,72,72)`；`on (32,32,32)` / `off (116,116,116)`），属两种合成下子控件文字 AA 与底色混合的 1 阶灰差，不是布局/内容错位。
- S1 已知边界：新表面目前只承担宿主客户区不透明底衬，GDI 装饰面/白卡背板仍走旧路径（S2 才挂 `IXamlSurface::GetRenderTarget`），此边界不影响上述等价结论。

---

## (b) flip 交换链与 GDI 子窗口的合成策略（事实 + 行号）

### b.1 现状事实（源码取证）

| 事实 | 位置 | 证据 |
|---|---|---|
| 主窗样式含 `WS_CLIPCHILDREN` | `PyMfcXaml\PyMfcWrap.cpp:59-62` | 扩展样式 `WS_EX_APPWINDOW \| WS_EX_COMPOSITED`；普通样式 `WS_OVERLAPPEDWINDOW \| WS_CLIPCHILDREN \| WS_CLIPSIBLINGS` |
| 主窗**未启用** `WS_EX_NOREDIRECTIONBITMAP` | 全 `*.h/*.cpp` 源码 grep `NOREDIRECTION` 无命中 | 主窗仍保留 GDI 重定向位图，GDI 子窗口可正常叠加 |
| 宿主子窗 `PyMfcHost` 也带 `WS_CLIPCHILDREN` | `PyMfcXaml\PyMfcHost.cpp:26-30` | ex=`0`；style `WS_CHILD \| WS_VISIBLE \| WS_CLIPCHILDREN \| WS_CLIPSIBLINGS` |
| 面板子窗同样带 `WS_CLIPCHILDREN` | `PyMfcXaml\PyMfcWrap.cpp:166-172` | `WS_CLIPSIBLINGS \| WS_CLIPCHILDREN` |
| 表面路径 OnPaint：`BeginFrame` → 不透明 Clear → `EndFrame`(Present) 后直接 return | `PyMfcXaml\PyMfcProcs.cpp:1082-1102` | `m_surface->BeginFrame(...)` → `m_surface->EndFrame(1)`；设备丢失才 `Recreate()`；**Present 之后没有对子窗强制 `RedrawWindow`** |
| 擦除背景在表面路径被短路 | `PyMfcXaml\PyMfcProcs.cpp:156-158` | `if (m_newSurface) return TRUE;` 背景由交换链后台 buffer 不透明 Clear 负责 |
| 原生子窗仍各自 `BeginPaint/EndPaint` | 自绘皮肤子窗 `PyMfcProcs.cpp:1500-1508`；新表 `Table.cpp:720-823`；动画宿主 `PyMfcHost.cpp:430-455` | 每个子 HWND 在自己 WM_PAINT 里 `BeginPaint` → 画 → `EndPaint`，S1 未改成节点 |
| 交换链参数 | `MfcXamlCore\XamlSurface.cpp:27,259,264,267,270`；Present `:102-103`；Clear `:77-79`；ResizeBuffers `:113-116` | `BufferCount=2`、`DXGI_SWAP_EFFECT_FLIP_DISCARD`、`B8G8R8A8_UNORM`、`CreateSwapChainForHwnd`、`DXGI_ALPHA_MODE_IGNORE`；`Present(syncInterval=1, 0)`；每帧不透明 Clear；Resize 先 `SetTarget(nullptr)` 再 `ResizeBuffers(2,…)` |

### b.2 选定方案（过渡期，对应 S1 现状，供 S2 前置）

1. **保持 `WS_CLIPCHILDREN` + 不透明 `ALPHA_MODE_IGNORE`**：交换链只接管宿主客户区背景与后续自绘节点区域；原生 Edit、滚动条、新表等真输入控件仍以子 HWND 覆盖在 D3D 表面之上；子窗区域由 `WS_CLIPCHILDREN` 裁剪，父表面不把子窗那块刷掉。
2. **本期不加 `WS_EX_NOREDIRECTIONBITMAP`**：该样式关掉窗口 GDI 重定向位图，会直接改变现有 GDI 子窗的合成/裁剪路径；当前 GDI 子窗仍占多数，提前打开风险不可控。待 S3 子窗基本转成 scene node、只剩少量输入覆盖层时再评估。
3. **Present 后不做全子窗 `RedrawWindow`**：子窗重绘仍由各自 WM_PAINT 驱动；表面只在自己 OnPaint 出帧。避免"父画→子失效→子再请求父画"的重绘循环（与 `PyMfcHost.cpp:27-30` 注释同一约束）。
4. 理由：本轮六场景像素等价（a.2）说明该共存方式下合成结果与旧路径一致；`=1` 下 `E_SEARCH` 焦点/`WM_CHAR` 已验证可用（S1 记录 §6）；子窗独立 BeginPaint 的开销属于 S2/S3 要消的成本，不在 S1 扩大改动。

### b.3 过渡期风险（S2 必须实测）

- **`WS_EX_COMPOSITED` 与 flip-model 的叠加**：主窗现仍带 `WS_EX_COMPOSITED`（自底向上合成子窗）。S2 把自绘控件挂到 D3D 表面后，要在真实拖动/平移/滚动帧上确认没有残留旧帧；若出现子窗与表面合成顺序问题，优先评估去掉 COMPOSITED，而不是退回 DC 渲染目标。
- **GDI 子窗与 D3D 后台 buffer 的区域交接**：子窗占住的矩形不在 D3D 帧内重画；窗口移动/缩放时若父表面补帧而子窗未跟上，可能露出后台 buffer 底色。`OnSize` 已走 `ResizeBuffers`（`Procs.cpp:1126-1127` → `XamlSurface.cpp:107-124`），S2 需补 resize 帧像素取证。
- **将来子窗转节点**：届时输入覆盖层（Edit/滚动条）保留 HWND；其余装饰/文字/表格视觉从子窗脱离后，再决定是否摘 `WS_EX_NOREDIRECTIONBITMAP`、是否去掉 `WS_EX_COMPOSITED`。本轮不预设该结论。

---

## (c) 610×80 横带根因：既有缺陷 ④（初始视图未 fit），非 S1 引入

### c.1 实测同状态数字（`=0`，showEditor 后）

- plan 模型尺寸：**6174 × 428**；`viewArea=(662, 755)`。
- 实际视图：`zoom=0.2000, ox=36, oy=334`。
- Python `fitView` 未钳前 `k = min(662/6174, 755/428)*0.92 = 0.09865`；钳到 `[0.2, 4.0]` → 0.2。
- C++ `fitPlanCanvas`：`kw=(662-8)/6174=0.10593`，`kh=(755-8)/428=1.74533`；`min(min(kw,kh),1.0)` 后钳 `<0.2 → 0.2`。
- 按下限 0.2 放大，模型高 `428*0.2 ≈ 85.6px`（超宽模型宽 `6174*0.2 ≈ 1234.8px`，被 662 视宽左右裁掉）——屏上只剩一条约 80~86px 高的压扁横带。

### c.2 两条钳制规则（行号 + 算式）

**C++ `PyMfcHost::fitPlanCanvas`** — `PyMfcXaml\PyMfcHost.cpp:3453-3472`：

- `:3464` `kw = (rc.right - 8) / w`
- `:3465` `kh = (rc.bottom - 8) / h`
- `:3466` `st.zoom = min(min(kw, kh), 1.0f)`（**上限 1.0**）
- `:3467` `if (st.zoom < 0.2f) st.zoom = 0.2f;`（下限 0.2，注释 HTML clampK [0.2,4]）
- `:3468-3471` `ox/oy` 居中后 `if (<0) =0`（**负偏移被钳回 0**）

**Python `App.fitView`** — `tests\mfc_xaml\pp_app.py:1180-1190`：

- `:1185` `k = min(aw/W, ah/H) * 0.92`（0.92 padding）
- `:1186` `k = max(0.2, min(4.0, k))`（**上限 4.0，下限 0.2**）
- `:1187-1188` `ox/oy` 居中，**未做 `<0 → 0` 钳制**
- `:1189` `setPlanView(CV, k, ox, oy)`

### c.3 为什么 `_base_frozen.png` 反而是过放大 + 左侧被裁

- 基线截图是**另一个应用状态**（非本次 showEditor 后的超宽模型状态）：该状态走 Python `fitView`，上限放到 4.0，允许 `k>1`，卡片被放大；同时 Python `ox/oy` 不钳负偏移，居中算出负 `ox` 后左侧被裁。两张图不是同一状态，不能直接逐像素对照。
- C++ `fitPlanCanvas` 上限 1.0 且 `ox/oy≥0`，不会出现过放大左裁；Python 路径允许 `k>1` + 负偏移，这正是两张截图外观相反的规则差。
- 两条路径在本次超宽模型状态下都被下限 0.2 兜住 → 同样是横带（既有抓帧 `=1`/`=0` 同状态逐像素 0 差，见 a.1）。

### c.4 结论

横带是**既有缺陷 ④：初始视图未真正 fit（超宽模型被 0.2 下限钳住）**，根因在 fit 钳制规则与视口/模型尺寸，**S1 未改这条链路**（本轮 C++ 零改动），不是 D3D11 新表面引入的问题。修复属后续布局/视图层事项，不在 S1 范围。

---

## (d) 一句话结论

S1 D3D11 新路径在六场景下与旧 GDI 路径像素等价（最高差异 0.0259%，且仅为状态栏文字 AA 灰阶）；**默认值继续保持 `PYARX_SURFACE=0`，本轮不切 1**；进入 S2 前需按 b.3 实测 flip 模型与现存 GDI 子窗的合成/裁剪稳定性。

---

## 附 1：输入探针修正（pp_real_test / pp_cursor_test）

### 修正前（探针自身 bug，非被测代码缺陷）

`pp_real_test.py` 旧 `send()`（`:56-58` 旧版）：

```python
def send(flags, data=0, x=0, y=0):
    user32.mouse_event(flags, 0, 0, data, None)
```

函数签名收了 `x,y`，实现却把 `dx/dy` 恒传 0。真实左键拖动步进调用
`send(0x0001, 0, dx, dy)`，鼠标相对位移恒为 0：探针记录 `wasDrag=1 moved=0`，
坐标 `(43,336) -> (43,336)`，平移视图不变。

### 修正后（断言一字未放宽，只让输入真实发生）

文件：`G:\PyArx\tests\mfc_xaml\pp_real_test.py`；改前备份 `docs\_bk6_\S1V_pp_real_test.py`。

- 拖动步进改为**绝对坐标 `SetCursorPos` 轨迹**：起点屏幕坐标 `(sx0,sy0)`，第 `i` 步
  `sx=sx0+160*i/6`、`sy=sy0+100*i/6`；`SetCursorPos` 后按当下 `ScreenToClient(cv)` 折回客户坐标，
  补一条带 `MK_LBUTTON` 的 `WM_MOUSEMOVE`（与 `pp_events.drag` 已验证同法）。
- 点击/滚轮仍走 `mouse_event`（不需要移动）；`send()` 不再承担移动。
- 新增 `force_front()`：`AttachThreadInput` 到前台线程 → `ShowWindow(SW_RESTORE)` →
  `BringWindowToTop` → `SetActiveWindow` → `SetForegroundWindow` → 保持 `HWND_TOPMOST`。
  无人值守会话里后台进程直接 `SetForegroundWindow` 常被系统挡回，挂输入队列后激活才稳。

**为何这套在无人值守会话里稳**：`SetCursorPos` 是系统级绝对坐标定位，与 `pp_cursor` 已通过的
`real_move` 同一路径；`mouse_event` 的相对 `dx/dy` 是 mickey，受指针加速/DPI 缩放影响，才是原先不稳的部分。
直接 `WM_MOUSEMOVE` 补发只保证宿主拖动状态一定收到，不替代真实光标移动。

### 前后对照（`=0`，软件 D2D）

| 项 | 修正前 | 修正后 |
|---|---|---|
| `pp_real_test` 结果 | 5 PASS / 1 FAIL（历史回归表）；本轮修复前复跑 2/6，前台被豆包 Chrome 窗压住时点击/滚轮/拖动全未命中 | **6/6 PASS** |
| 真实拖动坐标 | `(43,336) -> (43,336)`，`moved=0` | `(36,334) -> (196,434)`，视图 `oy 334→434`，探针 `wasDrag=1 lastHit=AT1-1` |
| 真实单击选中 | 历史项 | PASS `AT1-1`；详情浮层 `visible=True` |
| 真实滚轮 | 历史项 | 放大 `0.2000→0.2240`，缩回 `0.2000` |
| 前台窗口 | 跨进程激活被挡 | `GetForegroundWindow()` 等于目标 hwnd |

`pp_cursor_test.py`：本轮**未改代码**（与备份 `docs\_bk6_\S1V_pp_cursor_test.py` 哈希一致）。
它此前已用真实 `SetCursorPos`（`real_move`）+ 置顶前台，本轮单独复跑 **5/5 PASS**
（手形 `0x1001f`、表格上 `0x10003` 不挂手形、回画布恢复手形）。历史标注的 2 项"模拟
WM_MOUSEMOVE 不触发 WM_SETCURSOR"在当前构建已由真实光标路径消除；不存在需要放宽的断言。

### 备份与产物命名（`_s1v_` 前缀）

- 备份：`docs\_bk6_\S1V_pp_real_test.py`、`docs\_bk6_\S1V_pp_cursor_test.py`。
- 探针：`tests\mfc_xaml\_s1v_scene.py`、`tests\mfc_xaml\_s1v_compare.py`。
- 原始输出：`docs\_s1v_run0.txt` / `_s1v_run1.txt`（六场景抓帧日志）、`_s1v_scene_diff.txt`、
  `_s1v_real_run1..4.txt`、`_s1v_cursor_run1.txt`、`_s1v_select_solo1.txt`、
  `_s1v_regress1.txt`、`_s1v_regress2.txt`。

---

## 附 2：11 套回归两次重跑（`=0` 默认路径，软件 D2D）

回归脚本：`tests\mfc_xaml\_run_regression.ps1`（每套前杀 Python314 进程并等待）。

### 第一轮（`docs\_s1v_regress1.txt`）

| 套件 | PASS | FAIL |
|---|---:|---:|
| pp_test_all | 27 | 0 |
| pp_split_test | 13 | 0 |
| pp_cell_test | 15 | 0 |
| pp_event_test | 8 | 0 |
| pp_grid_test | 18 | 0 |
| pp_select_test | 4 | 3 |
| pp_real_test | 6 | 0 |
| pp_cursor_test | 5 | 0 |
| pp_click_test | 5 | 0 |
| pp_runtime_test | 9 | 0 |
| pp_plugin_test | 5 | 0 |
| **合计** | **115** | **3** |

### 第二轮（`docs\_s1v_regress2.txt`）

| 套件 | PASS | FAIL |
|---|---:|---:|
| pp_test_all | 27 | 0 |
| pp_split_test | 13 | 0 |
| pp_cell_test | 15 | 0 |
| pp_event_test | 8 | 0 |
| pp_grid_test | 18 | 0 |
| pp_select_test | 7 | 0 |
| pp_real_test | 6 | 0 |
| pp_cursor_test | 5 | 0 |
| pp_click_test | 5 | 0 |
| pp_runtime_test | 9 | 0 |
| pp_plugin_test | 5 | 0 |
| **合计** | **118** | **0** |

### 第一轮 3 项红的明细与定性（`pp_select_test`，首轮日志）

失败断言（三条链式，同一次卡片点击未生效引起）：

1. `FAIL 单击卡片被选中` — 期望 `AT1-1`，实际 `''`；点击 `(140,389)`；卡片客户区 `rect=(119,384,42,11)`，落点在矩形内。
2. `FAIL 详情浮层打开` — 期望 true，实际 `visible=False cur=''`。
3. `FAIL 详情显示的就是该节点` — 期望 `AT1-1`，实际 `''`。

证据与定性：
- 同轮后续用例 `点空白清空选中 ''`、`单击连线选中下游节点 AT1-1`、两张控件 PNG 导出均 PASS，说明画布命中/导出链路本身正常。
- 该探针走 `SendMessageW` 直发，不依赖前台/真实鼠标；失败点在首轮进程刚 `pump(100)` 后 `showEditor()` 再 `pump(250)`，卡片首帧视图/选中态可能尚未稳定（超宽模型 0.2 横带状态，卡片高仅 11px）。
- 复跑条件：`pp_select_test` 单独起新进程 **7/7 PASS**（`docs\_s1v_select_solo1.txt`）；全量第二轮 **7/7 PASS**。
- 结论：属无人值守会话的启动时序间歇，不是本轮探针修正引入、也不是被测 C++ 回归；断言未放宽。若后续全量跑再连续复现，应在探针首帧补一次视图就绪等待（仍不改期望值）。

### 口径备注

- 拖动分割条红线 ≤60 ms/步：本轮 split 套件 13/13 绿，未回退；S1 未触分割条 C++ 路径。
- 详情 ≤40 ms、画布平移闪烁、表格滚动闪烁、新表滚动条缺口：均为既有未解项，不在 S1 处理。
- 性能/闪烁结论均来自端到端抓帧与真实进程回归，未采用脚本 pump 预算内计时。
