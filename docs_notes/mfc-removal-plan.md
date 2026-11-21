# MFC 剥离计划（第 70 轮盘点）

> 盘点性质：只读。本轮不改源码、不换头、不构建。
> 口径：全仓 931 个源文件（.h/.hpp/.cpp/.cxx/.cc/.c/.inl），排除 `_attic`、`third_party`、`pylibs`、`tools/train`、`viewer/_sdk`、`res`、`tmp_*`、`docs`、`.git`、`build`、`X64`、`site-packages`、`node_modules`。
> 取证脚本：`G:\PyArx\tmp_audit\audit70.py`（类计数/热点文件）、`audit70b.py`（afx 包含/stdafx/vcxproj）、`audit70c.py`（方法与成员抽取）、`audit70_final.py`（终稿计数）；原始结果：`tmp_audit\api70_audit.json`、`api70_methods.json`、`api70_final.json`。每条数字均可由上述脚本复跑指回。

## 1. 结论

按实测调用面，shim 必须实现的 API 分四组：

| 组 | 必须实现命名方法数 | 构造/运算符/包装面 | 说明 |
|---|---|---|---|
| CString | 16 | 3 构造 + 赋值/`+`/`+=`/`==`/`operator LPCTSTR` 5 面 | 单宽（wchar_t）口径；`c_str`/`GetString` 双取值别名 |
| Geom（CRect/CPoint/CSize） | 10 | 3 结构体公共成员 + 构造 + `==`/`+`/`-`/`+=` | 成员直访 `left/top/right/bottom`、`x/y`、`cx/cy` |
| CDC | 17 | 包 HDC，Attach/Detach/GetSafeHdc | 实测全部走 `CDC*`/`CDC&` 透传，无 CPaintDC 绘制主干 |
| GDI（CPen/CBrush/CBitmap/CFont/CMenu） | CMenu 7 | CPen/CBrush/CBitmap/CFont 4 个薄包装类 | CFont 实测 0 调用，仅占位；CFont/CBitmap 基线数字实测不成立 |

统计里出现但实为少量个案：`CPaintDC` 3 处、`CBitmap` 2 处、`CMenu` 6 处、`CStringA` 2 处、`CSize` 6 处（全在 AdUi 调色板回调内）、`CClientDC` 0 处。`CFont` 实测 0 命中。

### 1.1 实测 vs 用户口径复核

| 类 | 实测调用点 | 实测文件数 | 用户口径 | 出入判断 |
|---|---|---|---|---|
| CString | 229 | 13 | 3638 | 口径严重偏高；实测 227 落在 UIDesign，2 落在 PyMfcXml |
| CDC | 21 | 9 | 552 | 偏高；基线漏列 CPen(4)/CBrush(13) |
| CRect | 230 | 21 | 438 | 偏高 |
| CPoint | 151 | 12 | 489 | 偏高 |
| CSize | 6 | 2 | 555 | 严重偏高；仅 `PyArxCore\PyPaletteSet.h/.cpp` |
| CBitmap | 2 | 1 | 69 | 偏高 |
| CFont | 0 | 0 | 19 | 实测 0，无需实现方法 |
| CMenu | 6 | 4 | 11 | 接近 |
| CPaintDC | 3 | 3 | 3 | 一致 |
| CClientDC | 0 | 0 | 0 | 一致 |
| CWnd | 68 | 14 | 134 | 偏高 |
| CPen | 4 | 1 | 基线未列 | 补入 GDI 组 |
| CBrush | 13 | 3 | 基线未列 | 补入 GDI 组 |
| CFrameWnd | 8 | 3 | 基线未列 | 独立 EXE/UIDesign，见 §5 |
| CWinApp | 12 | 5 | 基线未列 | 杂项宿主面 |

> 复核方式：`rg -w CString G:\PyArx`（同排除口径）= 229，与 Python 扫描一致；基线 3638 等数字在当前仓内无法复现，疑为旧快照或计入了 SDK 头/非源文本，本计划以实测为准。

### 1.2 按模块分布（实测）

| 模块 | 实测要点 |
|---|---|
| MfcXmlCore | 源文件代码 0 个 MFC 类；`CFrameWnd`/`CWinApp` 各 1 处均为注释（`XmlD2D.h:41`、`controls\CodeEditor.h:174`）；vcxproj 已 Not Using |
| MfcXmlFrame | CWnd 18、CRect 6、CDC 2、CPoint 2（`XFrameWnd.*`） |
| PyArxCore | CSize 6、CRect 4、CPoint 1，全在 `PyPaletteSet.*`（AdUi 耦合，见 §4-4） |
| PyMfcXml | CRect 54、CWnd 14、CPoint 9、CDC 7、CWinApp 5、CString 2、CPaintDC 1 |
| UIDesign | CString 227、CRect 162、CPoint 136、CWnd 36、CBrush 13、CDC 10、CMenu 6、CPen 4、CWinApp 4、CBitmap 2、CPaintDC 2、CFrameWnd 2 —— 最大热点 |
| tools | CFrameWnd 5、CRect 3、CDC 2、CWinApp 2（`tools\chatview\src\main.cpp` 独立 MFC EXE） |

## 2. 方法清单（必须 / 可延后 / 个案）

计数为仓内调用点次数（成员调用与静态调用合并）。

### 2.1 CString（XString.h）

| 方法/面 | 次数 | 典型文件 | step1 必须 |
|---|---|---|---|
| GetString | 32 | DesignerPaneSurfaceInput.cpp、DesignerPaneFeature.cpp | 是 |
| IsEmpty | 31 | DesignerPaneSurface.cpp、DesignerPaneSurfaceInput.cpp | 是 |
| Format | 20 | DesignCanvas.h、DesignerPaneSurface.cpp | 是 |
| CompareNoCase | 16 | DesignerPaneView.cpp | 是 |
| Left | 12 | DesignerPaneFeature.cpp | 是 |
| Mid | 11 | DesignerPaneFeature.cpp | 是 |
| Compare | 8 | DesignerPaneSurface.cpp | 是 |
| GetLength | 7 | DesignerPaneView.cpp | 是 |
| Empty | 7 | DesignerPaneView.cpp | 是 |
| Trim | 5 | DesignerPaneView.cpp | 是 |
| ReverseFind | 4 | DesignerPaneView.cpp | 是 |
| Find | 4 | DesignerPaneSurface.cpp | 是 |
| c_str（取值别名） | 25 | 各 UIDesign 文件 | 是 |
| GetBufferSetLength | 2 | PyMfcHost.cpp、PyMfcProcs.cpp | 是 |
| ReleaseBuffer | 2 | PyMfcHost.cpp、PyMfcProcs.cpp | 是 |
| Replace | 2 | DesignerPaneView.cpp | 是 |
| 构造：默认/拷贝/`const char*`/`const wchar_t*`/`std::string`·`std::wstring` 的 c_str | 多处 | DesignerPaneFeature.cpp、DesignerPaneInternal.h | 是 |
| `operator=`/`+`/`+=`/`==`/`operator LPCTSTR` | 多处 | DesignerPaneView.cpp、DesignerPaneFeature.cpp | 是 |
| GetBuffer（裸缓冲） | 0 直接调用 | — | 预留，随 GetBufferSetLength 一并暴露 |
| Right、TrimLeft、TrimRight、MakeLower、MakeUpper、GetAt、SetAt、LoadString、Collate、Delete、Insert、Remove、FindOneOf、SpanIncluding、SpanExcluding | 0 | — | 延后，不实现 |
| `CStringA` 窄别名 | 2 | DesignerPaneFeature.cpp:328 等 | 个案：保留窄 typedef 或改调用点 |

### 2.2 Geom（XGeom.h）

| 方法/成员 | 次数 | 典型文件 | step1 必须 |
|---|---|---|---|
| Width | 75 | DesignCanvas.h、DesignerPaneView.cpp | 是 |
| Height | 75 | DesignCanvas.h、DesignerPaneView.cpp | 是 |
| IntersectRect | 11 | DesignCanvas.h | 是 |
| IsRectEmpty | 7 | DesignCanvas.h | 是 |
| NormalizeRect | 6 | DesignCanvas.h | 是 |
| OffsetRect | 6 | DesignerPaneView.cpp | 是 |
| InflateRect | 5 | DesignCanvas.h | 是 |
| SetRect | 4 | DesignerPaneView.cpp | 是 |
| PtInRect | 4 | DesignCanvas.h | 是 |
| UnionRect | 1 | DesignCanvas.h | 是 |
| 成员 `left/top/right/bottom`（CRect）、`x/y`（CPoint）、`cx/cy`（CSize） | 直访数百处 | DesignCanvas.h、PyMfcHost.cpp | 是，公共成员直访 |
| 构造：默认、4 int、`(POINT,SIZE)`、拷贝；`operator==`/`+`/`-`/`+=` | 多处 | PyMfcHost.cpp、DesignerPaneView.cpp | 是 |
| 其余 CRect/CPoint/CSize 方法 | 0 | — | 延后 |

### 2.3 CDC（XDC.h）

| 方法 | 次数 | 典型文件 | step2 必须 |
|---|---|---|---|
| LineTo | 28 | DesignerTools.h | 是 |
| Rectangle | 26 | DesignerTools.h | 是 |
| MoveTo | 23 | DesignerTools.h | 是 |
| SetBkColor | 12 | PyMfcProcs.cpp、DesignerPaneView.cpp | 是 |
| SelectObject | 11 | DesignCanvas.h、DesignerTools.h | 是 |
| GetSafeHdc | 10 | PyMfcProcs.cpp、DesignCanvas.h | 是 |
| FillSolidRect | 10 | PyMfcProcs.cpp、DesignerPaneView.cpp | 是 |
| Detach | 9 | PyMfcProcs.cpp、DesignerPaneView.cpp | 是 |
| SetTextColor | 9 | PyMfcProcs.cpp、DesignerPaneView.cpp | 是 |
| SetBkMode | 6 | PyMfcProcs.cpp | 是 |
| Ellipse | 4 | DesignerTools.h | 是 |
| Attach | 2 | PyMfcProcs.cpp、DesignerPaneView.cpp | 是 |
| RoundRect | 2 | DesignerTools.h | 是 |
| DrawText | 2 | DesignerPaneView.cpp | 是 |
| BitBlt | 1 | DesignCanvas.h | 是 |
| SelectStockObject | 1 | DesignerTools.h | 是 |
| CreateCompatibleDC | 1 | DesignCanvas.h | 是 |
| DrawTextEx、GetTextExtent、FillRect、Arc、Polyline、Polygon、GetTextColor、GetBkColor、SaveDC、RestoreDC | 0 | — | 延后 |

### 2.4 GDI（XGdi.h）

| 类/方法 | 次数 | 典型文件 | 必须 |
|---|---|---|---|
| CMenu::CreatePopupMenu | 5 | DesignerPaneSurfaceInput.cpp、DesignCanvas.h、DesignerPaneView.cpp | 是 |
| CMenu::AppendMenu / AppendMenuW | 18（AppendMenu 15、AppendMenuW 3） | DesignerPaneSurfaceInput.cpp、DesignerPaneView.cpp、DesignCanvas.h | 是 |
| CMenu::TrackPopupMenu | 4 | DesignerPaneSurfaceInput.cpp、DesignCanvas.h | 是 |
| CMenu::CreateMenu | 1 | DesignerPaneView.cpp | 是 |
| CMenu::DestroyMenu | 1 | DesignerPaneView.cpp | 是 |
| CMenu::Detach | 1 | DesignerPaneView.cpp | 是 |
| CMenu::m_hMenu 成员直访 | 1 | DesignerPaneView.cpp:746 | 是，公开 HANDLE 成员 |
| CPen 构造 `(PS_SOLID, width, COLORREF)` | 2 | DesignerTools.h | 是，包 HPEN |
| CBrush 构造 `(COLORREF)`/`new CBrush(c)` | 多处 | DesignerPaneView.cpp/.h、DesignerTools.h | 是，包 HBRUSH |
| CBitmap | 2（仅 SelectObject 交换） | DesignCanvas.h:535/3180 | 是，包 HBITMAP |
| CFont | 0 | — | 占位薄包装 HFONT，不实现方法 |
| CPaintDC | 3 | PyMfcProcs.cpp、DesignCanvas.h、DesignerPaneView.cpp | step2 补 BeginPaint/EndPaint RAII |
| CClientDC | 0 | — | 不实现 |

## 3. 热点文件（按调用点排序）

### 3.1 CString（13 个文件，全列）

| 次数 | 文件 |
|---|---|
| 63 | UIDesign\DesignerPaneView.cpp |
| 46 | UIDesign\DesignerPaneSurface.cpp |
| 30 | UIDesign\DesignerPaneSurfaceInput.cpp |
| 29 | UIDesign\DesignerPaneView.h |
| 24 | UIDesign\DesignerPaneFeature.cpp |
| 11 | UIDesign\ControlCatalog.h |
| 10 | UIDesign\DesignerTools.h |
| 6 | UIDesign\ControlCatalog.cpp |
| 4 | UIDesign\DesignerPaneInternal.h |
| 3 | UIDesign\DesignCanvas.h |
| 1 | PyMfcXml\PyMfcHost.cpp |
| 1 | PyMfcXml\PyMfcProcs.cpp |
| 1 | UIDesign\PyMfcDesigner.cpp |

### 3.2 CDC（9 个文件，全列）

| 次数 | 文件 |
|---|---|
| 4 | PyMfcXml\PyMfcProcs.cpp |
| 3 | PyMfcXml\PyMfcHost.h |
| 3 | UIDesign\DesignCanvas.h |
| 3 | UIDesign\DesignerPaneView.cpp |
| 2 | tools\chatview\src\main.cpp |
| 2 | UIDesign\DesignerPaneView.h |
| 2 | UIDesign\DesignerTools.h |
| 1 | MfcXmlFrame\XFrameWnd.cpp |
| 1 | MfcXmlFrame\XFrameWnd.h |

### 3.3 CRect（21 个文件，前 20）

| 次数 | 文件 |
|---|---|
| 123 | UIDesign\DesignCanvas.h |
| 20 | PyMfcXml\PyMfcHost.cpp |
| 18 | PyMfcXml\PyMfcProcs.cpp |
| 15 | UIDesign\DesignerPaneView.cpp |
| 8 | PyMfcXml\PyMfcWrap.cpp |
| 7 | PyMfcXml\PyMfcHost.h |
| 5 | MfcXmlFrame\XFrameWnd.cpp |
| 5 | UIDesign\DesignerPaneSurfaceInput.cpp |
| 5 | UIDesign\DesignerPaneView.h |
| 5 | UIDesign\DesignerTools.h |
| 4 | UIDesign\DesignerPaneSurface.cpp |
| 3 | PyArxCore\PyPaletteSet.cpp |
| 3 | tools\chatview\src\main.cpp |
| 2 | UIDesign\DesignerShell.cpp |
| 1 | MfcXmlCore\XFrameWindow.h |
| 1 | MfcXmlFrame\XFrameWnd.h |
| 1 | PyArxCore\PyPaletteSet.h |
| 1 | PyMfcXml\PyMfcLoad.cpp |
| 1 | UIDesign\DesignerPaneFeature.cpp |
| 1 | UIDesign\DesignerShell.h |

### 3.4 CPoint（12 个文件，前 8）

DesignCanvas.h 71、DesignerPaneView.h 29、DesignerPaneSurfaceInput.cpp 22、DesignerPaneView.cpp 11、PyMfcHost.cpp 5、PyMfcHost.h 4、MfcXmlCore\XFrameWindow.h 3、DesignerTools.h 2；其余 XFrameWnd.cpp/.h、PyEdEditor.cpp、DesignerPaneFeature.cpp 各 1。

## 4. 结构性障碍

### 4.1 afxwin.h 传递包含链

- PCH 直拉（4 个 stdafx.h，全仓 638 个 TU 经 stdafx.h 间接拿到 MFC）：
  - `PyArxCore\stdafx.h`：afxwin.h + afxext.h + afxcmn.h
  - `PyArxEngine\stdafx.h`：afxwin.h + afxext.h + afxcmn.h
  - `PyMfcXml\stdafx.h`：afxwin.h + afxext.h + afxcmn.h
  - `UIDesign\stdafx.h`：afxwin.h + afxext.h + afxcmn.h + afxdlgs.h
- 裸 include（不经过 stdafx）：`MfcXmlCore\XFrameWindow.h`、`MfcXmlFrame\XFrameWnd.h`、`PyArxCore\dllmain.cpp`、`PyArxCore\PyEdEditor.cpp`、`PyArxEngine\dllmain.cpp`、`PyMfcXml\dllmain.cpp`（afxdllx.h）、`tools\chatview\src\main.cpp`。

### 4.2 消息映射

`BEGIN_MESSAGE_MAP`/`END_MESSAGE_MAP` 各 8 处、`DECLARE_MESSAGE_MAP` 8 处：
`MfcXmlFrame\XFrameWnd.h/.cpp`、`PyArxCore\PyPaletteSet.h/.cpp`、`PyMfcXml\PyMfcHost.h`（映射体在 `PyMfcProcs.cpp`）、`PyMfcXml\PyMfcWrap.h/.cpp`、`UIDesign\DesignCanvas.h/.cpp`、`UIDesign\DesignerPaneView.h/.cpp`、`tools\chatview\src\main.cpp`。

### 4.3 DYNAMIC 宏

`DECLARE_DYNAMIC` 3 处：`PyPaletteSet.h` ×2、`PyMfcHost.h`；`IMPLEMENT_DYNAMIC` 3 处：`PyPaletteSet.cpp` ×2、`PyMfcProcs.cpp`。

### 4.4 AdUi 耦合（硬阻塞，shim 兜不住）

`PyArxCore\PyPaletteSet.h/.cpp` 继承 `CAdUiPalette`/`CAdUiPaletteSet`，include `adui.h`、`aduiPaletteSet.h`、`aduiPalette.h`、`rxmfcapi.h`；覆盖回调 `GetMaximumSize`/`CalcFixedLayout`/`GetMinimumSize` 用 CSize。这套挂 AutoCAD 宿主，不能被 Win32 shim 替代；`CSize` 的 6 处实测全在此文件内。该工程在 AdUi 分支隔离前不能整体改 Not Using。

### 4.5 工程配置（vcxproj）

| 工程 | UseOfMfc | _AFXEXT |
|---|---|---|
| MfcXmlCore | 已 Not Using | 无 |
| MfcXmlFrame | Dynamic | 无 |
| PyArxCore | Dynamic | 预处理器定义 + dllmain.cpp 内 `#define` |
| PyArxEngine | Dynamic | dllmain.cpp 内 `#define` |
| PyMfcXml | Dynamic | 预处理器定义 + dllmain.cpp 内 `#define` |
| UIDesign | Dynamic | 无 |
| tools\ShxProbe\Ttf2Line | Not Using | 无 |

`afxdllx.h` 出现在 `PyArxCore\dllmain.cpp`、`PyArxEngine\dllmain.cpp`、`PyMfcXml\dllmain.cpp`。

### 4.6 杂项 Afx* 符号（实测）

`AfxMessageBox` 10（UIDesign 8、chatview 2）、`AfxRegisterWndClass` 4、`AFX_MANAGE_STATE` 7、`AfxGetApp` 1（`PyMfcCommon.h:95`）、`AfxThrow` 2（均注释）、`ASSERT` 1（注释）；源码内 MFC `TRACE` 0 命中（docs 下 `LOG_TRACE` 为同名宏误命中）。

## 5. 风险清单（每条一句判断）

1. **afxwin.h 传递包含**：638 个 TU 靠 4 个 stdafx.h 拿 MFC；换头必须先换 stdafx.h 再逐 TU，shim 头与 afxwin 不能混编——shim 可兜，但顺序是硬约束。
2. **DECLARE/IMPLEMENT_DYNAMIC**：shim 提供空宏即可关闭；唯独 `PyPaletteSet` 基类是 CAdUiPalette，随 §4.4 保留，不能随 shim 去 MFC。
3. **BEGIN/DECLARE_MESSAGE_MAP**：必须由 XWindowBase 的 WndProc thunk + 自研消息分发表替代；宏展开不能再保留 MFC 静态消息映射结构。
4. **CString 隐式 LPCTSTR 与宽窄字符**：工程为 UNICODE/_UNICODE，shim 走 wchar_t 单宽口径，Format 用 wprintf 族；2 处 `CStringA` 窄别名需保留 typedef 或改调用点；`%s`/`%S` 格式串按宽口径处理。
5. **AfxGetApp / AFX_MANAGE_STATE / AfxMessageBox / AfxRegisterWndClass**：shim 可兜底——模块句柄包装、空宏、Win32 `MessageBoxW`、`RegisterClassW`；`AfxThrow`/`ASSERT` 仅注释，不实现。
6. **GDI 对象所有权**：CPen/CBrush/CBitmap 包 HPEN/HBRUSH/HBITMAP，`SelectObject` 返回旧句柄、析构 `DeleteObject`；`CMenu::Detach` 转移 HMENU 所有权，shim 不得双重释放或泄漏。
7. **AdUi 调色板**：不可 shim；`PyPaletteSet` 在 AutoCAD 宿主方案另立前保留 MFC/AdUi 链接，PyArxCore 的 UseOfMfc 不能在隔离前整体关闭。
8. **CSize 延后性**：6 处全在 AdUi 回调；调色板隔离后 CSize shim 可最后补，不阻塞 step1。
9. **UIDesign 热点集中且含窗口子类**：CString 227/229、CRect 162/230、CPoint 136/151、CWnd 36/68 都在 UIDesign，且有两个 CWnd 子类与消息映射；不能提前单独换头，必须 step3 随 XWindowBase 一起搬。
10. **独立 MFC EXE**：`tools\chatview`（CFrameWnd/CWinApp + 消息映射）、`tools\train\progress_mfc`（排除口径外）不进 shim 范围，UseOfMfc 保留或退役单独决策。
11. **MfcXmlCore 复核项**：`XFrameWindow.h` 裸 include afxwin.h，需确认 `LiveResize` 实际用到的 CWnd 面；该工程源码其余部分已无 MFC 类，换头风险最低。

## 6. 三步路线

### Step 1：CString / Geom 全量换头

- 范围：UIDesign 的 `ControlCatalog.*`、`DesignerTools.h`、`DesignerPaneInternal.h`、`DesignerPaneFeature.cpp`、`DesignerPaneSurface.cpp`、`DesignerPaneSurfaceInput.cpp`、`DesignerPaneView.*`、`DesignCanvas.h` 中的字符串与几何用量；PyMfcXml 纯数据侧 `PyMfcHost.*`、`PyMfcProcs.cpp`、`PyMfcWrap.cpp`、`PyMfcLoad.cpp`。
- 动作：`XString.h`/`XGeom.h` 就位；上述 TU 的 MFC 头引用换 shim，调用点一行不动。
- 先做换算验证的最小 TU 候选：`UIDesign\DesignerTools.h`（内联头、方法集合集中）或 `PyMfcXml\PyMfcCommon.h`。
- 验收门：候选 TU 编过；整步 `/m:1` 构建 EXIT=0，不新增失败；既有像素/功能基线不退。
- 不碰：CDC、CWnd、消息映射、AdUi、vcxproj 的 UseOfMfc。

### Step 2：CDC / GDI 全量换头（绘制层）

- 涉及文件：`PyMfcXml\PyMfcProcs.cpp`、`PyMfcXml\PyMfcHost.h`、`MfcXmlFrame\XFrameWnd.*`、`UIDesign\DesignCanvas.h`、`UIDesign\DesignerPaneView.*`、`UIDesign\DesignerTools.h`、`UIDesign\DesignerPaneSurfaceInput.cpp`（CMenu 随 GDI 层同批）。
- MfcXmlCore 绘制层 `XmlCore`/`XmlDraw`/`XmlD2D`/`XmlControls`/`XmlDwgCanvasView`：实测代码 0 个 CDC 类，本步只复核不改动。
- 动作：`XDC.h`/`XGdi.h` 包 HDC/HPEN/HBRUSH/HBITMAP/HMENU；3 处 CPaintDC 补 BeginPaint/EndPaint RAII。
- 验收门：绘制层编过；离屏 `CreateCompatibleDC`/`BitBlt` 路径外观一致；既有 `t_gradient`/`t_box_shadow`/`t_flexpanel` 等像素基线不退。

### Step 3：CWnd → XWindowBase + 工程去 MFC

- 动作：XWindowBase（WndProc thunk + 消息分发表 + `DefWindowProc` 兜底）替换 `XFrameWnd`、`PyMfcHost`、`PyMfcTopWnd`、`DesignCanvas`、`DesignerPaneView` 的 CWnd 基类；消息映射宏转分发表；Afx* 杂项转 Win32。
- 各工程清理点：
  - **PyArxCore**：`stdafx.h` 删 afxwin/afxext/afxcmn；`dllmain.cpp` 删 afxdllx.h 与 `_AFXEXT` define；`PyEdEditor.cpp` 裸 afxwin 改 shim/Win32；vcxproj UseOfMfc 改 Not Using、预处理器去 `_AFXEXT`；`PyPaletteSet` AdUi 分支先隔离再降 UseOfMfc（§4.4/§7）。
  - **PyArxEngine**：`stdafx.h` 删 afx 三件；`dllmain.cpp` 删 afxdllx.h/`_AFXEXT`；vcxproj 改 Not Using。
  - **PyMfcXml**：`stdafx.h` 删 afx 三件；`dllmain.cpp` 删 afxdllx.h/`_AFXEXT`；vcxproj 改 Not Using 并去 `_AFXEXT`；`PyMfcHost`/`PyMfcWrap` 消息映射与 DYNAMIC 转 XWindowBase；`PyMfcCommon.h` 的 AfxGetApp 改模块句柄。
  - **MfcXmlFrame**：`XFrameWnd.h` 删 afxwin.h；基类换 XWindowBase；vcxproj 改 Not Using。
  - **MfcXmlCore**：已 Not Using；`XFrameWindow.h` 裸 afxwin.h 按 §5-11 复核后删。
  - **UIDesign**：`stdafx.h` 删 afxwin/ext/cmn/afxdlgs；两个 CWnd 子类转 XWindowBase；vcxproj 改 Not Using。
- 验收门：全仓 0 个 `afx*.h` 包含；除隔离后的 AdUi 调色板外，vcxproj 全部 Not Using；EXIT=0；`tools\chatview` 等独立 EXE 单列决策。

## 7. 第 70 轮台账

- 2026-11-20 | 第70轮：MFC 剥离只读盘点（未改源码/未构建）。实测 931 源文件：CString 229/13 文件、CDC 21/9、CRect 230/21、CPoint 151/12、CSize 6/2、CBitmap 2/1、CFont 0、CMenu 6/4、CPaintDC 3/3、CClientDC 0、CWnd 68/14；补录 CPen 4、CBrush 13、CFrameWnd 8、CWinApp 12。必须实现：CString 16 方法 + 3 构造/5 运算转换面、Geom 10 方法 + 3 结构体、CDC 17 方法、GDI CMenu 7 方法 + 4 薄包装（CFont 占位）。硬阻塞：PyPaletteSet 继承 CAdUiPalette/CAdUiPaletteSet（adui/rxmfcapi）不可 shim；4 个 stdafx.h 传递包含 afxwin；8 组消息映射 + 3 组 DYNAMIC 待 XWindowBase 承接。计划见 `docs_notes\mfc-removal-plan.md` | 下轮：Step1 落地 XString/XGeom shim，先换算最小 TU 编过再铺 UIDesign/PyMfcXml 数据侧。

## 8. 第 71 轮进度（MfcXmlCore 全模块接 shim，双编期策略 A）

### 8.1 本轮落地

- 双定义策略选 **A**：shim 四头以 `#if !defined(__AFXWIN_H__)` 早退——MFC 在场的 TU 沿用 MFC 类型，MFC 缺席的 TU 才定义 shim 同名类。理由：过渡双编期同一伞头被 MFC-present（PyMfcXml/UIDesign）与 MFC-free（MfcXmlCore）两类 TU 包含；方案 B（头内按在场切换类型）要改签名，方案 C（仅逐 TU 包含）覆盖不了 72 个控件头。
- 伞头接线：`MfcXmlCore\XmlCore.h` 接入 `win32shim/XString.h|XGeom.h|XDC.h|XGdi.h`，经 `XmlMfcControlBase.h` 覆盖 64 个控件 TU 与 `XmlCore.cpp`/`XmlSvgScene.cpp`。MfcXmlCore 生产代码除 `XFrameWindow.h` 外实测 0 个 MFC 类，无直含 afx 头可换。
- PCH 复核（4 个 stdafx.h 本轮均未换）：`PyMfcXml`（CWnd 宿主，Step3）、`PyArxCore`（AdUi 调色板硬阻塞 §4.4 + ObjectARX MFC 耦合）、`PyArxEngine`（arx 入口/afxdllx，Step3）、`UIDesign`（两个 CWnd 子类 + 消息映射，Step3）。
- shim 补面：`XGdi.h` 新增 `CPen(int nStyle,int nWidth,COLORREF)`、`CBrush(COLORREF)` 薄包装；`CMenu` 补 `CreateMenu`/`DestroyMenu`/`m_hMenu` 别名；`XDC.h` 补 `Ellipse`/`Arc`/`Polyline`/`Polygon`/`FillRect`/`BitBlt`/`SelectStockObject`/`SaveDC`/`RestoreDC`。未加 XCompat.h：MfcXmlCore 内 `ASSERT/VERIFY/TRACE/AfxThrow` 实测 0 命中。
- ABI 复核：`XML_CORE_API` 导出签名与 `IXmlMfcHost` 接口只出现 `std::wstring`/`std::vector`/`int`/`bool`/`COLORREF`/`HWND(void*)`/`XmlElement*`，无 CString/CRect/CPoint/CSize/CDC/GDI 包装类；per-TU 类型分歧不越 DLL 边界。三套件全绿是运行佐证，不构成导出表全量证明，Step3 收尾需再核一次导出符号。
- 构建：`PyMfcXml.vcxproj` Release x64 `/m:1`，74 个唯一 TU，EXIT=0，`: error` 0；告警仍为既有 C4251/C4275/C4244/C4996/C4005，win32shim 路径 0 告警。运行：`t_flexpanel` 57、`t_flex` 21、`t_input_font` 8 全绿。
- 陷阱记录：windows.h 的 `DrawText→DrawTextW` 宏同时别名化 `d2d1.h` 的 `ID2D1RenderTarget::DrawText`；shim 头**不得** `#undef DrawText`，否则 D2D 接口名与调用点宏状态错位（C2039）。

### 8.2 下一步（Step1 剩余 → Step2 → Step3）

- **Step1 剩余**：UIDesign 数据侧（`ControlCatalog.*`、`DesignerTools.h`、`DesignerPaneInternal.h`、`DesignerPaneFeature.cpp`、`DesignerPaneSurface.cpp`、`DesignerPaneSurfaceInput.cpp`、`DesignerPaneView.*`、`DesignCanvas.h`）与 PyMfcXml 纯数据侧（`PyMfcHost.*`、`PyMfcProcs.cpp`、`PyMfcWrap.cpp`、`PyMfcLoad.cpp`）逐 TU 换 shim 头；换前确认各 TU stdafx 包含顺序，guard A 兜底。
- **Step2**：CDC/GDI 绘制层全量换头；3 处 CPaintDC 补 BeginPaint/EndPaint RAII（现 shim 收 HWND，CWnd* 重载待 Step3）。
- **Step3**：XWindowBase（WndProc thunk + 消息分发表 + DefWindowProc 兜底）承接 `XFrameWnd`/`PyMfcHost`/`PyMfcTopWnd`/`DesignCanvas`/`DesignerPaneView`；消息映射宏转分发表；各工程 stdafx 删 afx、afxdllx/`_AFXEXT` 清理、UseOfMfc 改 Not Using。

### 8.3 AdUi 隔离两个方案取舍

- **方案甲（隔离保留，本轮倾向）**：`PyPaletteSet`/`PyApp` 继续链接 adui24/acui24 与 MFC，单列为 MFC 残留模块，PyArxCore 其余部分先去 MFC。优点：不动 AutoCAD 宿主回调基类（`CAdUiPalette`/`CAdUiPaletteSet`），风险最低；代价：仓内长期保留一份 MFC 工程，UseOfMfc 不能全仓清零。
- **方案乙（shim 替代 AdUi）**：Win32 自绘面板 + 自研布局替代调色板回调面（`GetMaximumSize`/`CalcFixedLayout`/`GetMinimumSize`）。优点：全仓去 MFC；代价：需复刻 AdUi 调色板在 AutoCAD 侧的停靠/标签页行为，工作量与回归面大，`rxmfcapi` 耦合未必剥净。
- 决策点：Step3 收尾前按 `PyPaletteSet` 实际回调面清单再定；本轮先按甲推进隔离。

## 9. 第 72 轮进度（XWindow.h 兼容层骨架 + CWnd 分组表）

### 9.1 本轮落地

- 新增 `win32shim/XWindow.h`，沿用策略 A（`__AFXWIN_H__` 在场即早退，MFC-present TU 不定义同名 CWnd）。伞头 `XmlCore.h` 接入。
- CWnd 薄包装 28 个方法/成员：`m_hWnd`、`GetSafeHwnd`/`operator HWND`/`Attach`/`Detach`/`FromHandlePermanent`、`Create`/`CreateEx`、`SetWindowText`/`GetWindowText`/`GetWindowTextLength`、`GetClientRect`/`GetWindowRect`/`ScreenToClient`/`ClientToScreen`、`Invalidate`/`InvalidateRect`/`UpdateWindow`、`ShowWindow`/`EnableWindow`/`SetWindowPos`、`SetTimer`/`KillTimer`、`SendMessage`/`PostMessage`/`GetParent`/`SetParent`/`GetDlgItem`/`GetDlgCtrlID`。
- 虚函数 34 个，只取 audit72.py 枚举到的真实 override 槽：`PostNcDestroy`/`PreTranslateMessage`/`OnCmdMsg`/`OnCreate`/`OnSize`/`OnEraseBkgnd`/`OnTimer`/`OnPaint`/`OnCtlColor`/`OnDrawItem`/`OnMeasureItem`/`OnSetCursor`/`OnLButtonDown`/`OnLButtonUp`/`OnLButtonDblClk`/`OnRButtonDown`/`OnMButtonDown`/`OnMButtonUp`/`OnMouseMove`/`OnMouseWheel`/`OnChar`/`OnKeyDown`/`OnContextMenu`/`OnNcHitTest`/`OnNcCalcSize`/`OnGetMinMaxInfo`/`OnEnterSizeMove`/`OnExitSizeMove`/`OnClose`/`OnDestroy`/`OnActivate`/`OnNcActivate`/`OnHScroll`/`OnNotify`；另析构与 `xshim_GetMsgMap` 虚函数各 1。
- 消息映射宏真展开：`DECLARE_MESSAGE_MAP`/`BEGIN_MESSAGE_MAP`/`END_MESSAGE_MAP`/`ON_WM_PAINT`/`ON_WM_SIZE`/`ON_WM_ERASEBKGND`/`ON_WM_CTLCOLOR`/`ON_WM_DRAWITEM`/`ON_WM_MEASUREITEM`/`ON_WM_SETCURSOR`/`ON_WM_LBUTTONDOWN`/`ON_WM_LBUTTONUP`/`ON_WM_LBUTTONDBLCLK`/`ON_WM_RBUTTONDOWN`/`ON_WM_MBUTTONDOWN`/`ON_WM_MBUTTONUP`/`ON_WM_MOUSEMOVE`/`ON_WM_MOUSEWHEEL`/`ON_WM_CONTEXTMENU`/`ON_WM_TIMER`/`ON_WM_CREATE`/`ON_WM_CLOSE`/`ON_WM_DESTROY`/`ON_WM_HSCROLL`/`ON_WM_NCHITTEST`/`ON_COMMAND`/`ON_NOTIFY`/`ON_MESSAGE`/`ON_CONTROL_RANGE`；静态表 + 一次查找，未命中落 `DefWindowProcW`；`DECLARE_DYNAMIC`/`IMPLEMENT_DYNAMIC`/`RUNTIME_CLASS` 空实现。
- 落点判定：两个候选 `MfcXmlCore\XFrameWindow.h`、`PyMfcXml\PyMfcWrap.h` 所属 TU 均 MFC-present（stdafx 先含 afxwin），策略 A 下换头等于没换；按预案走最小切片——MfcXmlCore 新增自检 TU `controls\XWindowShimCheck.cpp`（已进 vcxproj/filters），编译期 + 空句柄 API 可解析校验，未换任何真实 CWnd 文件。
- 陷阱记录：① **不得引 `windowsx.h`**——其 `SubclassWindow`/`CopyRgn` 宏与 MFC 成员名冲突，afxwin.h 只在 windowsx 先于它包含时才 `#undef`；后引会顶掉成员函数（C2059）。lparam 解码改用本地 `XSHIM_GET_X_LPARAM`/`XSHIM_GET_Y_LPARAM`/`XSHIM_WHEEL_DELTA` 小宏。② 消息表条目宏之间必须带逗号；取表自由函数需在类内 inline 调用前有前向声明。③ 类模板注册表特化不能落匿名 namespace（C2888），最终改固定名自由函数方案（同一 TU 一个映射类）。
- 导出面复核：`IXmlMfcHost` 与全部 `XML_CORE_API` 签名仍只有 `std::wstring`/`std::vector`/`int`/`bool`/`COLORREF`/`HWND(void*)`/`XmlElement*`/`xmlapp::XmlAppearance`；shim `CWnd*`/`CDC*` 只出现在 win32shim 内部，不越 DLL 边界。
- 构建：`build_audit_013345.txt`，74 个唯一 TU，EXIT=0，`: error` 0，win32shim 路径 0 告警；五个业务工程 UseOfMfc 保持 Dynamic 未改。运行：`t_flexpanel` 57、`t_flex` 21、`t_input_font` 8 全绿；`flex_demo` 截图最大色差 1/255（21642 像素 ±1，抗锯齿噪声，无结构差异）。

### 9.2 CWnd 分组表（Step3 换头顺序）

- **A 组：纯 CWnd 消息子集**（随所属工程 stdafx 去 afx 后优先换）
  - A1 `PyMfcXml\PyMfcWrap.h` / `PyMfcWrap.cpp`（`PyMfcTopWnd`：`OnSize`/`PostNcDestroy` + `ON_WM_SIZE`；`SubclassWindow` 绑定；换头前 shim 需补 `SubclassWindow`/`UnsubclassWindow`）。
  - A2 `UIDesign\DesignCanvas.h` / `DesignCanvas.cpp`（鼠标/键盘/计时器/`OnPaint`/`OnEraseBkgnd`/`OnSetCursor` 族）。
- **B 组：owner-draw / 字体 / CTLCOLOR / DRAWITEM / MEASUREITEM / 命令范围**
  - B1 `PyMfcXml\PyMfcHost.h` / `PyMfcHost.cpp`（`OnCtlColor`/`OnDrawItem`/`OnNotify`/`OnHScroll` + `ON_CONTROL_RANGE`/`ON_COMMAND_RANGE`；兼 `IXmlMfcHost` 实现）。
  - B2 `PyMfcXml\PyMfcProcs.cpp`（`IMPLEMENT_DYNAMIC` + 全套 ON_*，`ON_CONTROL_RANGE` 8、`ON_MESSAGE` 6）。
  - B3 `UIDesign\DesignerPaneView.h` / `DesignerPaneView.cpp`（`OnCtlColor`/`OnDrawItem`/`OnMeasureItem`/`OnMouseWheel`/`OnContextMenu`，跨 A/B 归 B）。
- **C 组：不可换**（AdUi / 框架壳，随工程整体处置或隔离保留）
  - C1 `PyArxCore\PyPaletteSet.h` / `PyPaletteSet.cpp`（`CAdUiPalette`/`CAdUiPaletteSet`，AdUi 硬阻塞，按 §8.3 方案甲隔离保留）。
  - C2 `MfcXmlFrame\XFrameWnd.h` / `XFrameWnd.cpp`（无框顶层框架壳：NC 消息族 + 系统循环 + DWM）。
  - C3 `UIDesign\DesignerShell.h` / `DesignerShell.cpp`（框架壳 `PreTranslateMessage` 转发，依附 XFrameWnd）。

### 9.3 下一步

- 73 轮先补 shim 缺口：`SubclassWindow`/`UnsubclassWindow`、`CScrollBar` 形参（现 `OnHScroll` 暂用 `CWnd*` 占位）、`OnCmdMsg`/`PreTranslateMessage` 链路、同一 TU 多映射类支持；再随 PyMfcXml 工程 stdafx 去 afx，A1 `PyMfcTopWnd` 首个真实换头。
- AdUi 取舍仍按 §8.3 方案甲；`XWindowBase` 与本层 `XWindow.h` 的合并口径在 Step3 收尾定。

## 10. 第 73 轮进度（A1 PyMfcTopWnd 换基类 + PCH 逐文件退出清单）

### 10.1 本轮落地

- XWindow.h 重构：类收进 `namespace xshim::XWindowBase`，全局不再声明 `class CWnd`，去掉 `__AFXWIN_H__` 早退守卫，无条件定义（命名空间隔离消除 ODR）；消息宏一律 `XSHIM_` 前缀，不再定义 `DECLARE_MESSAGE_MAP`/`BEGIN_MESSAGE_MAP`/`ON_WM_*`/`RUNTIME_CLASS`/`DECLARE_DYNAMIC` 等同名宏，避免顶掉 MFC 在场 TU 的宏。
- 自注册窗口类 `PyArxXShimWnd` + `ThunkProc` 分派到实例（`GWLP_USERDATA` 存 this，`WM_NCCREATE` 回填，`WM_NCDESTROY` 解绑）；未命中消息回落 `m_oldProc`（创建路径为 `DefWindowProcW`，子类化路径为换上前的原 proc）。补 `SubclassWindow(HWND)`/`UnsubclassWindow()`（存/还原旧 proc）；`FromHandlePermanent` 改全局 `HWND→XWindowBase*` 映射查询。
- 拆 TU：`PyMfcTopWnd` 声明落新头 `PyMfcXml\PyMfcTopWnd.h`（只含 shim 窗口层 + `<functional>`，`PyMfcHost` 前向声明）；实现落新 TU `PyMfcXml\PyMfcTopWnd.cpp`，vcxproj 单配 `<PrecompiledHeader>NotUsing</PrecompiledHeader>` + 空 `PrecompiledHeaderFile`，不吃 stdafx.h/afxwin.h。
- 宿主重排经 `relayout` 回调注入：`OnSize` 体在新 TU 内只调 `relayout(cx,cy)` 与 `resize(cx,cy)`；`MoveWindow(0,0,cx,cy,TRUE)` + `reflowXml()` 由 Wrap.cpp 在两个建窗站点注入 lambda，新 TU 不解引用 `PyMfcHost*`，从而不碰其 MFC 头链。
- `PyMfcWrap.h` 基类 `CWnd` → `xshim::XWindowBase`；共享头内**未**写 `using CWnd = ...` 别名（该头同时被 MFC 在场 TU 包含，别名会造成二义）。`PyMfcWrap.cpp` 最小删除：搬走消息表（`BEGIN/ON_WM_SIZE/END`）、`OnSize` 实现、`PostNcDestroy` 实现；其余一行不动。
- 删除第 72 轮自检 TU `controls\XWindowShimCheck.cpp`（含 vcxproj/filters 条目），本轮不再出自检 TU。
- 构建：`build_audit_022317.txt`，EXIT=0，`: error` 0；UseOfMfc 仍 Dynamic。回归：`t_flexpanel` 57/0、`t_flex` 21/0、`t_input_font` 8/0；`flex_demo` 12 项布局与基线逐行一致，截图最大色差 1/255（21642 像素 ±1，抗锯齿噪声）；嵌入窗体探针（消息父窗 + `create_embedded` + `cad_ui.xml`）EXIT=0，8 控件 ids 与 CAD 基线一致。
- 导出面复核：`IXmlMfcHost` 与全部 `XML_CORE_API` 签名无 CWnd*/CString/CRect/CPoint/CSize/CDC/GDI 包装类；Python 面 `Window.hwnd` 返回 HWND 的 POD 句柄（`long long`），不暴露 CWnd*。

### 10.2 PCH 逐文件退出清单（PyMfcXml 工程）

口径：仓内工程级 vcxproj 均未开 `/Yu` PCH（PyMfcXml/MfcXmlCore/MfcXmlFrame/PyArxCore/UIDesign 为 NotUsing，PyArxEngine 未设）；MFC 进入 TU 的实际途径是 `#include "stdafx.h"` 直含，或经 `PyMfcCommon.h → stdafx.h` 头链。“退出 PCH”在本仓即“TU 编译期不再见到 afxwin.h”。

**已不依赖 MFC 头（编译期不见 afxwin.h）**

| TU | 头链 | 备注 |
| --- | --- | --- |
| `PyMfcTopWnd.cpp` | `PyMfcTopWnd.h → win32shim/XWindow.h` | 本轮新建，per-file NotUsing + 空 PCH 文件；shim 侧独立 TU |
| `XmlBindings.cpp` | `XmlBindings.h`(pybind) → `XmlCore.h`/`XmlPowerPlan.h` | 纯核心层绑定，无宿主窗 |
| `RegisterPyMfcControls.cpp` | `XmlCore.h` + MfcXmlCore 控件头 | 核心层控件注册，MfcXmlCore 本就 Not Using MFC |

**仍 MFC-present（经 stdafx.h 直含或头链见到 afxwin.h）**

| TU | 进入途径 | 分组/去向 |
| --- | --- | --- |
| `dllmain.cpp` | 直含 `stdafx.h` | MFC 扩展模块初始化（`CDynLinkLibrary`/模块态），必须保留 |
| `PyMfcDwgGL.cpp` | 直含 `stdafx.h` | DWG GL 绘制，待 Step2/3 |
| `Table.cpp` | 直含 `stdafx.h` | 原生表格，待 B 组换头 |
| `PyMfcBind.cpp` | `PyMfcCommon.h`/`PyMfcWrap.h` 链 | Python 绑定面，随宿主侧退出 |
| `PyMfcLoad.cpp` | `PyMfcCommon.h`/`PyMfcWrap.h` 链 | guiload 加载器，随宿主侧退出 |
| `PyMfcOffscreen.cpp` | `PyMfcCommon.h` 链 | 注释称不依赖 Host，但仍经 Common 见 MFC；可先断 Common 链做纯核心切片 |
| `PyMfcHost.cpp` | `PyMfcHost.h → Common → stdafx.h` | B1，`IXmlMfcHost` 实现 + owner-draw |
| `PyMfcProcs.cpp` | `PyMfcHost.h` 链 | B2，全套 ON_* 消息族 |
| `PyMfcWrap.cpp` | `PyMfcWrap.h → Common → stdafx.h` | 见 10.3 保留原因 |

### 10.3 为什么 PyMfcWrap.cpp 留在 MFC PCH

- 该 TU 包含 `PyMfcWrap.h → PyMfcHost.h`（`PyMfcHost : public CWnd, public IXmlMfcHost`）→ `PyMfcCommon.h → stdafx.h(afxwin.h)`；TU 内多处持有 `m_host` 并调用其 MFC 继承方法（`MoveWindow`/`GetSafeHwnd`/`reflowXml` 等），同时以 MFC `CRect` 等类型参与宿主布局/绘制链。
- 整个 TU 退出 PCH 会同时拖着 B1 `PyMfcHost`（仍 MFC）与整套 MFC 宿主面，本轮边界内不可行。干净路径是先把纯顶层窗部分（`PyMfcTopWnd`）拆成 shim 独立 TU（本轮已做）；`Wrap.cpp` 只保留 MFC 宿主侧实现，待 B1/B2 换头、`PyMfcHost` 脱离 CWnd 后再整体退出。

### 10.4 未实现点 / 下一步

- shim 缺口：`CScrollBar` 形参（`OnHScroll` 现以 `XWindowBase*` 占位）；`OnCmdMsg`/`PreTranslateMessage` 链路未接；同一 TU 多映射类仍共用全局 `xshim_msg_map_entries` 固定名（第二个映射类需改每类一函数）。
- 取表自由函数为外部链接（非 inline）：/O2 下 inline 副本会被优化掉、外部引用 unresolved（LNK2019）；后续多映射类时一并重构。
- 嵌入窗体 `cad_form_test.py` 需 CAD 进程内 `PYRUNFILE` 运行；本轮以消息父窗独立探针走通同一 `create_embedded` + `guiload` + 列表接口路径（8 控件），CAD 进程内复核留待下一轮。
- 下一步按 §9.2 分组：A2 `UIDesign\DesignCanvas.*`，其后 B1/B2。

## 11. 第 74 轮进度（宏别名收口 + A2 受阻 + 无头 CAD 复核）

### 11.1 宏别名收口
- `MfcXmlCore/win32shim/XWindow.h` 末尾追加受保护裸名别名块：`DECLARE_MESSAGE_MAP` / `BEGIN_MESSAGE_MAP` / `END_MESSAGE_MAP` / `ON_WM_PAINT` / `ON_WM_SIZE` / `ON_WM_ERASEBKGND` / `ON_WM_CTLCOLOR` / `ON_WM_DRAWITEM` / `ON_WM_DESTROY` / `ON_WM_CLOSE` / `ON_WM_LBUTTONDOWN` / `ON_WM_MOUSEMOVE` / `ON_WM_MOUSEWHEEL` / `ON_WM_SETFONT` / `ON_COMMAND` / `ON_NOTIFY` / `ON_MESSAGE`，逐个 `#ifndef` 映射到对应 `XSHIM_` 宏；MFC 在场 TU 里 `afxwin.h` 先定义同名宏，`#ifndef` 跳过，别名不生效、不串味。
- 同步补 `XSHIM_ON_WM_SETFONT() { WM_SETFONT, 0 }`。
- `PyMfcTopWnd.cpp` 消息表改回裸名（`BEGIN_MESSAGE_MAP` / `ON_WM_SIZE()` / `END_MESSAGE_MAP()`）；共享头 `PyMfcTopWnd.h` 类体仍保留 `XSHIM_DECLARE_MESSAGE_MAP()`（被 MFC TU 经 Wrap.h 广泛 include，裸名会在 MFC TU 展开成 MFC 宏，与 shim 基类不匹配）。
- 构建证据：`log/build_audit_031950.txt` EXIT=0、`: error` 0。

### 11.2 A2 DesignCanvas 本轮未换头（受阻）
- 阻塞一：跨 TU ODR/布局致命。`DesignCanvas.h`（namespace `uidesign`，3000+ 行内联方法）被 `DesignerPaneView.h:18` include；`DesignerPaneView.cpp`（MFC-present TU，`stdafx.h` L14）L219 `new DesignCanvas()`、L272 `Invalidate()`、L419 `GetWindowRect`、L524/538/540 直访 `m_canvas->m_hWnd`、L557/1352 `GetSafeHwnd`、L917/1053/1165/1176/1231/1499/1536 调 `setDocument/setSelected/setSelectionSet/clearSelection`、L1736/1827 直访 `m_hWnd/zoom`。若头做 `#if PYARX_NO_MFC` 双分支，两 TU 看到的成员布局（MFC CDC/CBitmap vs shim）与 vtable 不一致，跨 TU 构造/调用即堆损坏。要换必须连同 B3 `DesignerPaneView` 一起换，或把 pane view 对 canvas 的直调全部改为 HWND Win32 调用（属 B3 接口重构），均超出“只改这一个”边界。
- 阻塞二：shim 缺口。`CDC::SelectObject` 需 `CBitmap*` 重载（现收/返 HGDIOBJ）；`CreateCompatibleDC` 需 `const CDC&` 重载（现只收 HDC）；无 `AfxRegisterWndClass` 对应；`GetParent()` MFC 回 `CWnd*`、shim 回 `XWindowBase*`；基类虚调用 `CWnd::OnSetCursor/OnSize/OnTimer/OnChar/OnKeyDown` 需分支改写；消息表 `ON_WM_CHAR/KEYDOWN/TIMER/SETCURSOR/RBUTTONDOWN/MBUTTONDOWN/MBUTTONUP/LBUTTONUP/LBUTTONDBLCLK` 裸名别名本轮未补（`XSHIM_` 本体大多已存在）。
- 本轮未改 `DesignCanvas.*`、未动 vcxproj；无新增 ClCompile 配置。

### 11.3 无头 CAD 复核结论
- 环境：`accoreconsole.exe` 起得来，`PyArxEngine.crx` arxload 成功，`PYRUNFILE` 通道可用；`PyArxCore`、`PyMfcXml` 在 CAD 内嵌 Python 中 import 均 ok（探针 `tmp_access/headless73_inner.py`，结果 `log/headless73_probe.json`）。
- 阻塞：无头会话无主框架，`PyApp.mainWindow` 返回 0（`IsWindow(0)=false`）；`create_embedded(parent=0,...)` 在 C++ 侧 `!::IsWindow(parent)` 校验失败，抛 `RuntimeError("PyMfc embedded window create failed")`。
- 结论：嵌入窗体路径在无头 CAD 环境不可复核；缺有效顶层父 HWND（accoreconsole 无 UI 主框架）。`tests/cad/cad_form_test.py` 本体本轮未跑通（环境性失败，非代码回归）；未用独立探针替代宣布通过，需在带交互 CAD 会话内复核。

### 11.4 剩余分组计数（第 74 轮后）
- A 组：剩余 1（A2 DesignCanvas，受阻，待 B3 捆绑换头或 pane view 接口重构）。
- B 组：3（B1 PyMfcHost、B2 PyMfcProcs、B3 DesignerPaneView）。
- C 组：3（C1 PyPaletteSet AdUi 隔离、C2 XFrameWnd、C3 DesignerShell）。

### 11.5 回归（本轮）
- 构建 `log/build_audit_031950.txt`：EXIT=0，`: error` 0。
- `t_flexpanel` PASS=57 FAIL=0；`t_flex` PASS=21 FAIL=0；`examples/flex_demo/demo.py` EXIT=0。

### 11.6 未实现点
- A2/B3 捆绑换头，或 pane view 直调改 HWND Win32 调用；shim 缺口清单见 11.2。
- CAD 进程内嵌入窗体复核待交互会话（无头缺主窗 HWND）。
- 消息表 `ON_WM_CHAR/KEYDOWN/TIMER/SETCURSOR/RBUTTONDOWN/MBUTTONDOWN/MBUTTONUP/LBUTTONUP/LBUTTONDBLCLK` 裸名别名待补。

## 12. 第 75 轮：单一无分支口径定调 + A2 前置修复（A2 未换头）

### 12.1 单一无分支 + 头内只允许 POD（后续 B 组统一口径；§13.1 修正）
- 换头 TU 的头直接 include `win32shim/XWindow.h`，基类无条件 `xshim::XWindowBase`，头内不得出现任何 `#ifdef`/`#if PYARX_NO_MFC` 双分支。
- 为什么双分支会 ODR/堆不一致：同一头被 MFC-present TU（仍吃 stdafx/afxwin）与 shim TU 同时 include；条件分支让两个 TU 看到不同的成员布局（MFC CDC/CBitmap vs shim 包装）与不同 vtable，跨 TU `new`、成员直访、虚调用即按错误布局访问，堆损坏。命名空间隔离（`xshim::`）只解决符号冲突，不解决布局分歧；要同一套布局，跨 TU 出现的成员必须是 POD 句柄（HWND/HDC/HBITMAP/int/bool/stdlib）或两个 TU 都解析到同一套 shim 类型。
- **口径修正（第 76 轮，详见 §13.1）**：上轮本条只立了「无分支」，被证伪一半——无分支 + 命名空间隔离仍挡不住头内写死 MFC/shim 包装类型带来的两套布局；补齐的另一半规则是「共享头只允许 POD」：成员/签名只能用 HDC/HBITMAP/HPEN/HBRUSH/HFONT/POINT/RECT/SIZE/COLORREF/HWND/`std::wstring`，包装类型下沉到 .cpp 内本地构造。

### 12.2 本轮 shim 前置修复（均为自保，不改 A2 源码）
- `XWindow.h` 顶部 `#include <windows.h>` 后 `#undef SubclassWindow`/`#undef CopyRgn`：`windowsx.h` 经 `XFrameWindow.h`/`CodeEditor.h` 在 `afxwin.h` 之后二次引入，把这两个名字定义成函数式宏，顶掉 `XWindowBase` 同名成员声明（C2146/C3646 等 30 个，仅 MFC-present TU 触发）。
- `XGdi.h` 的 `#undef AppendMenu` 移进 `#if !defined(__AFXWIN_H__)` 守卫内：MFC 在场 TU 必须保留 winuser.h 的 `AppendMenu→AppendMenuW` 宏；宏被提前摘掉后，MFC `CMenu` 成员（预编译后即 `AppendMenuW`）在裸名调用点解析不到（C2039 ×15，`DesignerPaneSurfaceInput.cpp`）。
- `XDC.h` 补 `CDC::CreateCompatibleDC(const CDC*)` 重载（实测画布 `m_memDC.CreateCompatibleDC(&dc)`，源 DC 取 `GetSafeHdc()`）。

### 12.3 实测 GDI 调用清单（DesignCanvas.h/.cpp，按 grep 定名，未铺满）
- 已覆盖/本轮补：`CDC::GetSafeHdc`、`CreateCompatibleDC(&dc)`（本轮补 `const CDC*` 重载）、`SelectObject`（HGDIOBJ 句柄式）、`BitBlt(const CDC*)`；`CBitmap::GetSafeHandle/DeleteObject/CreateCompatibleBitmap(const CDC&)`；`CMenu::CreatePopupMenu/AppendMenuW/TrackPopupMenu`。
- 画布未出现、本轮未补：`SelectObject(CPen*/CBrush*/CFont*)` 类型重载、`CFont::CreateFontIndirect`、`CDC::GetDeviceCaps`、`CDC::DeleteDC`（画布用裸 `::DeleteDC` Win32 调用）。
- 转换时 `CBitmap* oldBmp = SelectObject(&m_memBmp)` 这类类型化选择/换回，按 `XmlBackBuffer.h` 既有模式改 HGDIOBJ 句柄式，不在 shim 里补 CGdiObject* 体系。

### 12.4 A2 工程归属与本机可编性
- `DesignCanvas.h/.cpp` 属 `UIDesign\UIDesign.vcxproj`（MFC Dynamic，Application `.exe`，链 `MfcXmlFrame.lib`/`MfcXmlCore.lib`）；不依赖 ObjectARX/AdUi SDK，本机工具链 v145/MFC/WinSDK 在位，可单独 MSBuild 编起。
- 基线修复后 `log/build_uidesign_042342.txt` EXIT=0、`: error` 0（修复前基线 45 个 error：XWindow.h 宏冲突 30 + SurfaceInput AppendMenu 15）。
- 跨 TU 包含方：`DesignerPaneView.cpp`、`DesignerPaneSurface.cpp`（均 MFC-present）+ `DesignCanvas.cpp`。

### 12.5 A2 未换头：剩余阻塞与前置切片
- 阻塞一（成员布局 ODR）：类成员 `CDC m_memDC`、`CBitmap m_memBmp` 在 MFC TU 解析为 MFC 包装、shim TU 解析为 shim 包装，sizeof/布局不一致。前置切片：成员改 `HDC m_memDC`/`HBITMAP m_memBmp` POD，需要 DC 时在方法内局部包 `CDC`。
- 阻塞二（回调 ABI）：`std::function` 成员签名带 `CPoint`/`CRect` 值类型（`onCreateNode/onDrawBegin/onDrawEnd/onContextMenu/onCtrlDragCopy`），跨 TU 的 `std::function` 目标类型不一致。前置切片：签名改 `POINT`/`RECT`/`std::wstring` POD。
- 阻塞三（方法改写清单，换头时逐条落）：`create()` 去掉 `AfxRegisterWndClass`（shim `CreateEx` 自注册 `PyArxXShimWnd`）；`CWnd::Create` → `XWindowBase::Create`；`CWnd::OnSetCursor/OnSize/OnTimer/OnChar/OnKeyDown` 基类调用删除（shim 虚槽空实现，thunk 直接分派 override）；`OnSetCursor` 形参 `CWnd*` → `XWindowBase*`；`GetParent()->PostMessage` 改 `::GetParent(m_hWnd)` + `::PostMessageW`；`ScreenToClient` 走 shim 同名成员。
- 消息表裸名别名待补：`ON_WM_CHAR/KEYDOWN/TIMER/SETCURSOR/RBUTTONDOWN/MBUTTONDOWN/MBUTTONUP/LBUTTONUP/LBUTTONDBLCLK`（`XSHIM_` 本体大多已存在，别名块待扩）。

### 12.6 本轮构建与回归
- `log/build_audit_042507.txt`（PyMfcXml）：EXIT=0，`: error` 0。
- `log/build_uidesign_042342.txt`（UIDesign）：EXIT=0，`: error` 0。
- `t_flexpanel` PASS=57 FAIL=0；`t_flex` PASS=21 FAIL=0；`examples/flex_demo/demo.py` EXIT=0。
- DesignCanvas 真实构造探针本轮未跑：基类尚未换（仍 MFC `CWnd`），未用独立探针替代宣布通过；待 12.5 前置切片完成、换头编过后，再起 GUI 父窗 `new` canvas、SetBounds/Invalidate/pump/close 验证 `m_hWnd` 有效无堆崩溃。

### 12.7 剩余分组计数（第 75 轮后）
- A 组：剩余 1（A2 DesignCanvas，阻塞见 12.5）。
- B 组：3（B1 PyMfcHost、B2 PyMfcProcs、B3 DesignerPaneView）。
- C 组：3（C1 PyPaletteSet AdUi 隔离、C2 XFrameWnd、C3 DesignerShell）。

### 12.8 未实现点
- A2 前置切片（成员 POD 化 + 回调签名 POD 化）尚未落；换头、cpp NotUsing PCH、真实构造探针均待该切片之后。
- CAD 进程内嵌入窗体复核仍待交互会话（无头缺主窗 HWND，见 11.3）。

## 13. 第 76 轮：跨世界头清单审计 + 共享头 POD 规则 + DesignCanvas POD 样本

### 13.1 规则定调（对 §12.1 的修正）
- 上轮「单一无分支定义」被证伪一半：命名空间隔离（`xshim::`）只解决符号冲突，不解决跨 TU 布局分歧；头里只要写死两世界不同构的包装类型，无分支也会出两套布局。
- 成立规则：**共享头只允许 POD**。被 MFC-present TU 与 MFC-free TU 同时 include 的头，类布局与签名里不得出现 `CString/CRect/CPoint/CSize/CDC/CBitmap/CPen/CBrush/CFont/CMenu` 等两世界布局不同构的包装类型。
- POD 替换口径：成员用 `HDC/HBITMAP/HPEN/HBRUSH/HFONT` + 自持 `POINT/RECT/SIZE`（windows.h，两世界同一类型）；虚函数/回调参数用 `RECT&/POINT/COLORREF/HWND`；字符串用 `const wchar_t*` 或 `std::wstring`；CDC/GDI 用法落 .cpp 内本地构造——「头 POD 化 + 实现各自本地用」，不改行为。
- **签名也必须是 POD，不只是成员（第 77 轮补）**：POD 化的审查范围覆盖成员**与函数签名**（含 `std::function` 回调形参、消息处理虚槽、自由函数参数）；只清成员、签名仍留 `CPoint/CRect/CDC*/CString`，跨 TU 的 `std::function`/虚调用目标类型仍不一致，ODR 风险不解除。grep 自证时两类都要扫。
- 同构例外：`CPoint/CRect/CSize` 派生自 `POINT/RECT/SIZE`，sizeof 跨世界同构，布局安全；`CString` 内部 `std::wstring`，与 MFC `CString` 不同构，属危险类型。

### 13.2 跨世界头清单（/showIncludes 双编译交集，只读审计）
- 方法：MSVC `/showIncludes /E` 两次预处理；样本 A = `UIDesign\DesignerPaneView.cpp`（MFC-present，`/MD` `_AFXDLL`）；样本 B 并集 `PyMfcTopWnd.cpp` + `XmlBindings.cpp`（MFC-free）。脚本与结果：`log\audit_cross_world76.ps1`、`log\parse_cross_world76.py`、`log\cross_world76.json`（原始 `inc_mfc_view.txt` / `inc_free_topwnd.txt` / `inc_free_bindings.txt`）。
- 交集仓库头共 10 个：
  - POD-clean 核心头 5：`XmlAppearance.h`、`XmlCore.h`、`XmlDraw.h`、`XmlMatrix4.h`、`XmlPowerPlan.h`（grep 无 shim/MFC 类型）。
  - shim 守卫双定义本体 5：`XDC.h`、`XGdi.h`、`XGeom.h`、`XString.h`、`XWindow.h`（规则针对的对象；其命中类型为守卫内 fallback 定义，MFC 在场 TU 早退不解析）。
- 最危险 5 个（按命中包装类型，行号见 JSON）：
  - `XString.h`：`CString` 签名 16 + 体 28（内部 `std::wstring`，与 MFC `CString` 不同构，头号危险）。
  - `XDC.h`：`CDC` 签名 2/体 7；`CPoint` 签名 1；`CSize` 体 3；`CString` 签名 1。
  - `XGdi.h`：`CBitmap/CPen/CBrush/CFont/CMenu` 体包装定义；`CDC` 签名 1。
  - `XGeom.h`：`CRect/CPoint/CSize` 签名 + 体（派生 `POINT/RECT/SIZE`，布局同构，安全）。
  - `XWindow.h`：`CDC` 签名 2；`CPoint` 体 11（窗口层回调点，换头时随基类一并处理）。
- `DesignCanvas.h` 本轮仅 A 侧 include，尚非跨世界头；换头后将进入交集，故先做 POD 样本。

### 13.3 DesignCanvas.h POD 化（A2 前置切片第一刀；基类仍 `public CWnd` 未换）
- 成员：`CDC m_memDC` → `HDC m_memDC = nullptr`；`CBitmap m_memBmp` → `HBITMAP m_memBmp = nullptr`。
- 签名：`paintD2D(CDC&, int, int)` → `paintD2D(HDC hdc, int, int)`；`OnPaint` 调用点改传 `dc.GetSafeHdc()`。
- 实现落裸 Win32：`CreateCompatibleDC/CreateCompatibleBitmap/SelectObject/BitBlt` 句柄式；析构补 `::DeleteObject(m_memBmp)` + `::DeleteDC(m_memDC)`（原 CDC/CBitmap 析构内部回收的等价物）。
- 本轮未动：基类仍 `public CWnd`；`OnEraseBkgnd(CDC*)` 形参随基类换头再改；`std::function` 回调签名仍带 `CPoint/CRect` 值类型（ABI POD 化留下一切片）；`CPen/CBrush/CFont` 成员与 helper 在 B3 / `DesignerTools.h` 侧，本轮未动。

### 13.4 探针（真实构造 DesignCanvas）
- 链接形态：独立 vcxproj 手链接在本环境 MFC 窗口路径起不来（裸 CWnd `Create` 静默失败、`AfxRegisterWndClass` fastfail，属探针骨架问题，非画布回归）；最终临时把 theApp TU 替换进 UIDesign 工程，复用已验证可用的基线 MFC 引导/链接/资源。探针经真实 `newDocument` 路径 `new DesignCanvas()` + `create` + `MoveWindow`，再取画布裸指针复核 SetBounds/Invalidate/pump/close。收口已还原 `PyMfcDesigner.cpp` 与 `DesignerPaneView.h`（临时 `probeCanvas()` 访问口一并撤掉）。
- 结果 `log\probe_canvas76.json`：`create=1`、hwnd 有效（`isWindow=1`）、`moved=1`、`pumped=1`，进程 EXIT=0；marker `log\probe76_marker.txt`：`shell-ok → doc-loaded → moved → invalidated → pumped → json-written → close-posted`。
- 探针坑（记录防再踩）：`ccs=UTF-8` 打开的宽定向流上跑窄 `fprintf` 触发 UCRT fastfail；`DestroyWindow` 后 `delete shell` 二次释放 fastfail；关闭走 `PostMessage(WM_CLOSE)` 交回 MFC 正常生命周期。

### 13.5 构建与回归（本轮）
- UIDesign `log\build_uidesign_054242.txt`：EXIT=0、`: error` 0；PyMfcXml `log\build_audit_054249.txt`：EXIT=0、`: error` 0。
- `t_flexpanel` PASS=57 FAIL=0（`log\test76_t_flexpanel.txt`）；`t_flex` PASS=21 FAIL=0（`log\test76_t_flex.txt`）；`examples\flex_demo\demo.py` EXIT=0（`log\test76_demo.txt`）。

### 13.6 剩余分组计数（第 76 轮后）
- A 组：剩余 1（A2 DesignCanvas 基类换头；POD 成员切片已落，回调签名 POD 化 + 基类/XWindowBase 替换未做）。
- B 组：3（B1 PyMfcHost、B2 PyMfcProcs、B3 DesignerPaneView）。
- C 组：3（C1 PyPaletteSet AdUi 隔离、C2 XFrameWnd、C3 DesignerShell）。

### 13.7 未实现点
- DesignCanvas：`OnEraseBkgnd(CDC*)` 形参、`std::function` 回调 `CPoint/CRect` 值签名 POD 化；基类 `public CWnd` → `XWindowBase` 替换；cpp NotUsing PCH。
- B3 `DesignerPaneView.h` 的 `CBrush` 成员（`m_brushPanel/m_brushEdit/m_brushGroup/m_brushByColor`）与 `OnCtlColor/OnEraseBkgnd` 的 `CDC*` 形参；`DesignerTools.h` 内联 `DrawRounded/DrawToolIcon` 的 `CDC&/CPen/CBrush/CString`。
- CAD 进程内嵌入窗体复核仍待交互会话（无头缺主窗 HWND，见 11.3）。

## 14. 第 77 轮：destroyed=0 定性（时序）+ shim 预备件；A2 仍未换头

### 14.1 destroyed=0 定性：探针写盘时序，产品代码未改
- 判据落实：探针走 theApp TU 临时替换路线（用完还原，grep 自证 `probeCanvas/CProbeApp/ProbeHook/g_canvas` 仓库源码 0 残留）。`WH_CALLWNDPROC` 钩子按 hwnd 记录 `WM_DESTROY/WM_NCDESTROY` 计数；`ExitInstance` 复核三个产品窗 `IsWindow`。
- 结论依据（marker `log\probe77_marker.txt`，JSON `log\probe_canvas77.json`）：canvas/shell/view 三窗 `ncdestroy` 各 1 次（`ncdestroy canvas=1 shell=1 view=1`），`ExitInstance` 时 `alive canvas=0 shell=0 view=0`，进程 `RUN_EXIT=0`、无访问违规/WER。phase-1 JSON 写于 `WM_CLOSE` 之前（`destroyed=0, isWindowAfter=1`），phase-2 写于 `ExitInstance`（`destroyed=1, isWindowAfter=0`）——`destroyed=0` 是 JSON 先于关闭写盘所致，非重复释放。
- 旁证：钩子上 `destroy/ncdestroy n=2` 的窗类为系统 `IME`/`MSCTFIME UI`（第二次 DestroyWindow 时类名已取空），非产品对象；源码侧 shell/canvas/视图路径无 `PostNcDestroy` override、无 `delete this`（仅 `DesignCanvas` 析构内 `delete m_d2dCv` 成员所有权）。本轮产品代码一行未改。

### 14.2 shim 预备件（`XWindow.h`，双工程编译验证绿）
- 虚槽 POD 化：`OnEraseBkgnd(CDC*)` → `OnEraseBkgnd(HDC)`；`OnLButtonDown/Up/DblClk/RButtonDown/MButtonDown/MButtonUp/MouseMove/MouseWheel/OnContextMenu/OnNcHitTest` 形参 `CPoint` → `POINT`；`xshim_Dispatch` 解码同步改 `POINT pt` / `HDC`。现有 shim 派生方仅 `PyMfcTopWnd`（只 override `OnSize/PostNcDestroy`），不受影响。
- 补 `XWindowBase` 方法：`SetCapture()`/`ReleaseCapture()`、`MoveWindow(int,int,int,int,BOOL)` 与 `MoveWindow(LPCRECT,BOOL)` 两重载。
- 补裸名消息宏别名：`ON_WM_TIMER/SETCURSOR/RBUTTONDOWN/MBUTTONDOWN/MBUTTONUP/LBUTTONUP/LBUTTONDBLCLK/CHAR/KEYDOWN`（`XSHIM_ON_WM_CHAR/KEYDOWN` 本体宏一并补齐），均 `#ifndef` 守卫，MFC 在场 TU 不串味。
- `UIDesign.vcxproj` 对 `DesignCanvas.cpp` 显式单配 `PrecompiledHeader=NotUsing` + 空 `PrecompiledHeaderFile`（工程级本就 NotUsing，此为对齐 `PyMfcTopWnd.cpp` 既有做法的显式标注）。

### 14.3 A2 未换头：阻塞清单（换头时按「类::方法(签名)」逐条落）
- 基类：`DesignCanvas : public CWnd` → `public xshim::XWindowBase`；头 `#include "stdafx.h"` 摘除，改引 `XWindow.h`。
- 签名待 POD 化（头内仍 MFC 包装）：
  - `DesignCanvas::create(CWnd* parent, UINT id)` → `create(HWND, UINT)`；体去 `AfxRegisterWndClass`，走基类 `Create`。
  - `DesignCanvas::OnEraseBkgnd(CDC*)` → `(HDC)`；`OnSetCursor(CWnd*, UINT, UINT)` → `(xshim::XWindowBase*, UINT, UINT)`。
  - 鼠标/键盘族：`OnMButtonDown(UINT, CPoint)`、`OnMButtonUp(UINT, CPoint)`、`OnLButtonDown(UINT, CPoint)`、`OnMouseMove(UINT, CPoint)`、`OnRButtonDown(UINT, CPoint)`、`OnLButtonUp(UINT, CPoint)`、`OnLButtonDblClk(UINT, CPoint)` → `POINT`。
  - 内部方法：`zoomAt(CPoint, short)`、`arcEndpoints(const XmlElement*, CPoint&, CPoint&) const`、`lineEndpoints(const XmlElement*, CPoint&, CPoint&) const`、`handlePoints(const XmlElement*, CPoint[8]) const`、`hitHandleAny(CPoint, XmlElement*&)`、`hitHandle(XmlElement*, CPoint)`、`beginDrag(int, int, CPoint, bool)`、`finishJig(CPoint)`、`finishBand(CPoint)`、`beginMoveDragNoUndo(CPoint)`、`docPt(CPoint) const`、`hitTestRoot(XmlElement*, CPoint, Path&)`、`hitTest(XmlElement*, CPoint, CPoint, Path&)`、`activateControl(XmlElement*, CPoint)`。
  - 回调成员：`onCreateNode/onDrawBegin std::function<bool(CPoint)>`、`onDrawEnd std::function<void(CRect)>`、`onContextMenu std::function<void(XmlElement*, CPoint)>`、`onCtrlDragCopy std::function<void(const Path&, CPoint)>`。
  - 几何 helper：`drawRoundedCv(XmlCanvas&, const CRect&, COLORREF, COLORREF, int)`、`clearDarkenCv(XmlCanvas&, const CRect&, bool, int)`、`applyBorderStyleCv(XmlCanvas&, const CRect&, COLORREF, COLORREF, ...)`、`drawScrollBarsCv(XmlCanvas&, const CRect&, const std::wstring&)`、`drawTextCv(XmlCanvas&, const CRect&, const std::wstring&, COLORREF, ...)`、`dashedRectCv(XmlCanvas&, const CRect&, COLORREF)`、`selRectOf(XmlElement*, CPoint, CRect&)`、`gripCv(XmlCanvas&, CPoint, bool)`、`drawSelectionCv(XmlCanvas&, XmlElement*, CPoint, bool, ...)`、`arcBoxFromArc(double, double, double, int, int, CRect&)`、`arcGeom(const CRect&, int, int, ...)`、`arcPoints(const CRect&, int, int, int)`；自由函数 `drawNode(XmlCanvas&, XmlElement*, CPoint origin, ...)`。
  - 成员待 POD：`m_lineA/m_lineB/m_ctrlCopyPt/m_dragOrigin/m_jigStart/m_bandStart/m_panStart/m_zoomAnchor`（CPoint）、`m_grpBox/m_jigRect/m_bandRect`（CRect）。
- 体内部件：`OnPaint` 的 `CPaintDC dc(this)` 改裸 `BeginPaint/EndPaint`；`CWnd::OnSetCursor/OnSize/OnTimer/OnChar/OnKeyDown` 基类委托删除（shim 虚槽空实现）；`SetCapture/ReleaseCapture` 走 14.2 新 shim 方法；`GetParent()` 返回类型 `CWnd*` → `XWindowBase*`；3 处 `CString`（`create` 注册类名、jig/band 尺寸与相交文案）改 `std::wstring`/窄缓冲。
- 牵连头（换头前先清）：`DesignerTools.h` 顶部 `#include "stdafx.h"`，内联 `DrawRounded(CDC&, const CRect&, ...)`、`DrawToolIcon(CDC&, const CRect&, const CString&)` 用 `CPen/CBrush/CDC&/CString`；画布头未调用其任何符号，可从 `DesignCanvas.h` 摘除该 include，MFC view TU 自行包含。`XmlControls.h` 等其余被含头的 MFC 表面需再跑一次 `/showIncludes` 交集复核。
- 跨 TU 调用点（与 B3 捆绑）：`DesignerPaneView.cpp` 的 `create(this, nextCtrlId())`（父参改 HWND）、`MoveWindow(rc)`（走 shim `MoveWindow(LPCRECT)`）、4 个回调 lambda 形参 `CPoint/CRect` 随签名改 `POINT/RECT`。

### 14.4 构建与回归（本轮）
- UIDesign `log\build77_uidesign.txt`：EXIT=0、`: error` 0；PyMfcXml `log\build77_pymfcxml.txt`：EXIT=0、`: error` 0。
- `t_flexpanel` PASS=57 FAIL=0（`log\test77_t_flexpanel.txt`）；`t_flex` PASS=21 FAIL=0（`log\test77_t_flex.txt`）；`examples\flex_demo\demo.py` EXIT=0（`log\test77_demo.txt`）。

### 14.5 剩余分组计数（第 77 轮后）
- A 组：剩余 1（A2 DesignCanvas；14.2 预备件已落，14.3 签名/成员/体内部件与牵连头未做）。
- B 组：3（B1 PyMfcHost、B2 PyMfcProcs、B3 DesignerPaneView）。
- C 组：3（C1 PyPaletteSet AdUi 隔离、C2 XFrameWnd、C3 DesignerShell）。

## 15. 第 78 轮：A2 正式挂起；B1 公共面/POD 接口切片（PyMfcHostApi.h）

### 15.1 A2 挂起（DesignCanvas）
- 状态：**正式挂起**，后续轮次不再尝试 A2 换头。
- 挂起理由（成本 > 收益）：`DesignerTools.h` 是设计器私有内联头，内联 `DrawRounded/DrawToolIcon` 等用 `CDC&/CPen/CBrush/CRect/CString`；POD 化它等于重写设计器绘制层。而发布物只出 `PyMfcXml` / `MfcXmlCore`，UIDesign 不随发布，A2 的 MFC 残留不进交付面。
- 恢复条件：**仅当要发布 UIDesign 时再做**；恢复时按 14.3 阻塞清单逐条落（基类换 XWindowBase、签名/成员 POD 化、cpp NotUsing PCH、真实构造探针）。

### 15.2 B1 审计：3 个绑定 TU 对 PyMfcHost 公开面的真实命中（只读 grep + /showIncludes 复核）
- 样本 TU：`XmlBindings.cpp`、`RegisterPyMfcControls.cpp`（MfcXmlCore\controls）、`PyMfcTopWnd.cpp`。
- 命中表：
  - `XmlBindings.cpp`：PyMfcHost 公开成员/方法/字段 **0 命中**；绑定面全是 XmlRuntime/XmlElement/PowerPlan/SvgScene/XmlControlRegistry/颜色解析。
  - `RegisterPyMfcControls.cpp`：**0 命中**；只操作 `XmlControlRegistry` 工厂表（`isKnown/registerFactory`）与 `IXmlControl*` 工厂 lambda。
  - `PyMfcTopWnd.cpp`：**0 方法调用**；仅头 `PyMfcTopWnd.h` 持有前向声明指针槽 `host`（写入仅 MFC-present `PyMfcWrap.cpp:53/182`，全仓无读取，属遗留写死成员）。
- MFC 包装类型（CString/CRect/CDC/CBitmap/CWnd*）在三 TU 调用路径上：**0**。闭包 `/showIncludes` 复核：无 afxwin/afxext/afxc/afxdlgs/stdafx/PyMfcHost.h/PyMfcWrap/PyMfcCommon/XFrameWindow.h；闭包内 `CString/CDC/CPaintDC/CBitmap` 仅为 win32shim 守卫 fallback 本体（XString.h/XDC.h/XGdi.h，13.2 既有双定义设计），TU 代码路径未引用。
- 审计脚本/日志：`log\audit78_includes.ps1` + `inc78_controls.txt/_E.i`；`log\audit78_topwnd.ps1` + `inc78_topwnd.txt/_E.i`；bindings/topwnd 沿用 76 轮 `inc_free_bindings.txt`/`inc_free_topwnd.txt` 复核。

### 15.3 B1 切片落地：`PyMfcXml\PyMfcHostApi.h`（只放 POD）
- 新头 `PyMfcHostApi.h`：`#include <windows.h>`，声明抽象接口 `IPyMfcHostApi`；操作集合按 15.2 真实命中定，未照抄整个 PyMfcHost。
  - 操作：`hostHwnd()`（取宿主顶层窗 HWND，POD；跨世界替代 MFC `GetSafeHwnd` 入口）。
- `PyMfcHost` 继续 `: public CWnd, public IXmlMfcHost`，追加 `public IPyMfcHostApi`；`hostHwnd()` 声明落 `PyMfcHost.h`、实现落 `PyMfcHost.cpp`（`return GetSafeHwnd();`）；基类未换。
- `PyMfcProcs.cpp` 一行未动。
- `PyMfcTopWnd.h`：摘除 `class PyMfcHost;` 前向声明，改 include `PyMfcHostApi.h`；成员 `host` 类型 `PyMfcHost*` → `IPyMfcHostApi*`。
- 另两个 TU 无需改 include：`XmlBindings.cpp` 保持 `XmlBindings.h/XmlCore.h/XmlPowerPlan.h/XmlSvgScene.h/pybind11/stl.h`；`RegisterPyMfcControls.cpp` 保持 `RegisterPyMfcControls.h/XmlCore.h/XmlMfcControlBase.h` + 控件头。

### 15.4 B1 拆解进度（公共面已切/未切）
- 已切：跨世界宿主指针槽类型脱离 `PyMfcHost`（→ `IPyMfcHostApi*`）；POD 接口头独立成文件；三绑定 TU 闭包不含 `PyMfcHost.h`/afx 头。
- 未切：`PyMfcHost.h` 主体仍 `CWnd` 派生 + MFC 面（OnCtlColor/OnEraseBkgnd/OnMouseMove 等 `CDC*/CPoint` 形参、`CRect` 成员/签名、`py::object` 回调面）；`PyMfcProcs.cpp` 全套 ON_* 消息族（B2）；`PyMfcWrap.cpp`/`PyMfcLoad.cpp`/`PyMfcBind.cpp` 仍经 `PyMfcWrap.h → PyMfcHost.h` 链持 `PyMfcHost` 值/指针并调 MFC 继承方法。
- 剩余分组计数（第 78 轮后）：A 组 0（A2 挂起，不计入活跃剩余）；B 组 3（B1 主体未切、B2 PyMfcProcs、B3 DesignerPaneView）；C 组 3（C1 PyPaletteSet、C2 XFrameWnd、C3 DesignerShell）。

### 15.5 构建与回归（本轮）
- PyMfcXml `log\build78_pymfcxml.txt`：EXIT=0、`: error` 0（仅既有 sscanf C4996 警告）。
- Python 冒烟 `log\test78_smoke.txt`：import PyMfcXml + `M.Window` 建宿主窗 + show/pump/close，EXIT=0，hwnd 有效。
- `t_flexpanel` PASS=57 FAIL=0（`log\test78_t_flexpanel.txt`）；`t_flex` PASS=21 FAIL=0（`log\test78_t_flex.txt`）；`examples\flex_demo\demo.py` EXIT=0（`log\test78_demo.txt`）。

### 15.6 未实现点
- Api 接口操作集仍只有 `hostHwnd()`；宿主其余 POD 候选操作（attach/detach/尺寸/repaint/属性读写）待其消费 TU 脱离 MFC 世界后再迁入，本轮未预抄。
- B1 主体（CWnd 基类、MFC 形参/成员、py::object 面）、B2、B3、C1–C3 未动；UIDesign 工程本轮未构建（改动仅限 PyMfcXml 侧头/源，UIDesign 不 include `PyMfcHost.h`）。
- CAD 进程内嵌入窗体复核仍待交互会话（无头缺主窗 HWND，见 11.3）。
