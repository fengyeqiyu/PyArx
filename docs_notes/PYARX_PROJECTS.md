# PyArx 项目职责地图

> 依据：`PyArx.sln` 的 7 个工程 + 目录内实际源码/工程文件（2026-11-14 实读）。
> 供 AI 在改代码前先确定"这段逻辑归哪个工程"。

## 0. 全局分层（一句话）

```
Python 业务脚本
   ├─ PyArxCore.pyd         CAD 数据库/几何/编辑器 API（只能在 acad.exe 内 import）
   └─ PyMfcXml.pyd          MFC 界面宿主（XML → 控件树）+ MfcXmlRuntime(py) 复合控件
              │
              ├─ MfcXmlCore.dll     控件库 + XML 运行时（无 MFC 依赖）
              ├─ MfcXmlFrame.lib    无框/有边顶层窗口框架（静态库）
              └─ Table.cpp / PlanGL 表格与配电画布绘制

PyMfcXml.pyd ← acad.exe 通过 PyArxEngine.arx 装载（提供 PYRUNFILE/PYLOAD/PYEXEC 命令）
UIDesign.exe / Viewer.exe ← 同一批 MfcXmlCore 绘制函数（设计器 / 预览器）
```

依赖事实（取自 vcxproj）：`PyMfcXml` → `MfcXmlCore`（ProjectReference）；`UIDesign` → `MfcXmlFrame.lib + MfcXmlCore.lib`；
`Viewer` → `MfcXmlCore.lib`；`MfcXmlCore` 只链 msxml6/ole32/gdi32/user32；`PyArxCore` 只链 ObjectArx 库。

## 1. PyArxCore（DynamicLibrary → `X64\Release\PyArxCore.pyd`）

**作用**：ObjectArx 2024 的 pybind11 绑定层，把 C++ 类库暴露成 Python 子模块。不含界面逻辑。

- 模块入口：`PyArxBinder.cpp`（`PYBIND11_MODULE`），子模块：`Db` / `Ge` / `Gi` / `Ed` / `Ap` / `Core` / `Utility` / `Debug` / `Rx` / `Enum`。
- 主要文件族：`PyDb*.cpp`（数据库与实体，约 200 个类）、`PyGe*.cpp`（几何）、`PyEd*.cpp`（aced* 编辑器函数、选择集、Jig、UCS）、`PyGi*`（图形接口）、`PyAp*`（Application/文档/侧栏）、`PyUt/PyColor/PyXdata/PyResbuf`（工具）。
- 侧栏：`PyPaletteSet.cpp` 包装 `CAdUiPaletteSet`；页内控件宿主由 GUI 层回填。
- 关键常量：`HotModuleName.h`、`stdafx.h`（Python 头 + ObjectArx 头整合）。
- **边界**：只能在 `acad.exe` 进程内 import；独立 python 进程 import 会 access violation。

## 2. PyArxEngine（DynamicLibrary → `PyArxEngine.arx` / `.crx`）

**作用**：AutoCAD 的 ARX 宿主外壳，负责"把 Python 引擎拉起来 + 注册 CAD 命令"。本身不写业务。

- 入口：`acrxEntryPoint.cpp` —— `kInitAppMsg` 只注册命令（加载期零重量），首次命令才 `PyEngine_Init`（懒初始化）。
- 引擎：`PyEngine.cpp`（`Py_Initialize`、sys.path、pylibs、脚本执行）、`PyEngineC.cpp`（C 接口给 Core.pyd 用）、`PyRedir.cpp`（stdout 重定向到命令行）。
- 命令绑定：`PyRxCmd.cpp` / `PyCmdBinder.cpp` / `PyDispatchBinder.cpp` / `PyAppDispatcher.cpp`。
- 提供命令：`PYLOAD`（选脚本）、`PYRUNFILE`（执行指定 .py，cadmcp 走这条）、`PYEXEC`（执行输入代码）。
- 卸载顺序（重要）：清 palette → 清 `sys.modules` → `Py_FinalizeEx` → FreeLibrary pyd 句柄。
- **边界**：同一个 acad 进程只能装一次；重新编译 pyd 需先卸载 ARX 再 `pyreload`。

## 3. MfcXmlCore（DynamicLibrary → `MfcXmlCore.dll`）

**作用**：XML 声明式 UI 的运行时核心 —— 解析、布局、样式、模板、绑定、控件注册表、绘制。设计器/预览器/运行时三方共用，保证"四处一致"。

- 核心：`XmlCore.cpp/.h`（解析 + 布局 + 控件注册 + 属性反射）、`XmlAppearance.h`、`XmlTheme.h`。
- 绘制：`XmlDraw.h`、`XmlLeafPaint.h`、`XmlBackBuffer.h`、`XmlSurface.cpp`、`XmlD2D.h`（D2D11 后端）、`XmlSvgScene.cpp`（nanosvg）。
- 几何：`XmlGeometry.cpp`（Boost.Geometry）。
- 复合/特色：`XmlPowerPlan.cpp`（配电规划图）、`XmlPowerCanvasView.h`、`XmlRibbon.h`（Ribbon/Backstage）、`XFrameWindow.h`。
- 控件：`controls\Xml*.cpp/.h`（约 60 个：Button/TextBox/ComboBox/ListView/TableView/TreeView/Grid/Panel/Tab/Menu/Slider/ProgressBar/Calendar/CodeEditor/Dial/Tumbler…）。
- 控件契约：`controls\XmlEvents.h`（事件名契约表）、`controls\XmlTypes.h`（属性表类型）、`controls\XmlMfcTags.h`（标签名）、`controls\RegisterPyMfcControls.cpp`（注册入口）。
- **边界**：不依赖 MFC；控件实体由宿主（`PyMfcHost` / 设计器）提供工厂实现。

## 4. MfcXmlFrame（StaticLibrary → `MfcXmlFrame.lib`）

**作用**：顶层窗口框架（Qt `ApplicationWindow` + `FramelessWindowHint` 的等价物），被 UIDesign / Viewer 共用。

- `XFrameWnd.h/.cpp`：`FrameFrameless`（客户区铺满、四边 6px 交系统缩放、标题栏交系统移动、DWM 圆角剥离）/ `FrameNative`（系统标题栏）。API：`CreateFrame` / `SetContent` / `SetCaptionDrag` / `SetMinTrackSize` / `SetFrameBackground`。
- `XFrameWindow.h`：无框命中测试与系统循环通知的基础件。
- **边界**：只做窗口外壳，不管内容；每个程序自己是"调用者"，关闭询问与快捷键按业务重写。

## 5. PyMfcXml（DynamicLibrary → `PyMfcXml.pyd`）

**作用**：MFC 界面宿主 + 对 Python 暴露的 UI 模块（`PYBIND11_MODULE(PyMfcXml)`）。把 XML 变成真实窗口与控件，并把控件事件回派给 Python。

- `PyMfcBind.cpp`：**全工程唯一的**模块定义，Python 接口都在这。
- `PyMfcHost.cpp/.h`：`class PyMfcHost`（控件创建、皮肤、事件、画布、XML 桥）。
- `PyMfcProcs.cpp`：窗口过程（全控件自绘、配电画布鼠标状态机、可编辑 Grid、滚轮安装）。
- `PyMfcWrap.cpp/.h`：`PyMfcTopWnd` / `PyMfcWindow` / `PyMfcPanel` / `PyMfcStore`（顶层窗口与面板句柄层）。
- `PyMfcLoad.cpp`：`guiload` —— XML 文件 → 控件树（布局运行时 + 控件工厂）。
- `PyMfcOffscreen.cpp`：BGRA→PNG(WIC)、D2D 配电图、SVG 离屏导出。
- `Table.cpp/.h`：可编辑表格 Grid 的实现；`PyMfcPlanGL.cpp`：配电图 GL 渲染；`XmlBindings.cpp`：XmlRuntime/XmlElement 绑定。
- 回调约定：XML 里 `on_click="fn"`，或加载后 `child.click = fn`（两种等价，可混用）。
- **边界**：pyd 依赖 MFC 模块态；改接口只动 `PyMfcBind.cpp`，不要另开模块定义。

## 6. UIDesign（Application → `UIDesign.exe`）

**作用**：可视化 UI 设计器。打开 `designer_layout.xml` 做壳（顶栏 + 画布 + 属性表 + 对象树 + XML 代码坞），在画布上拖放控件、改属性、所见即所得。

- 外壳：`DesignerShell.cpp`（用 MfcXmlFrame 的 XFrameWnd）、`designer_layout.xml`。
- 画布：`DesignCanvas.h`（169 KB，控件拖放/框选/缩放几何）、`DesignerPaneSurface.cpp` / `DesignerPaneSurfaceInput.cpp`（绘制与输入）、`DesignerPaneView.cpp`（视图/布局/属性面板块）。
- 目录与元数据：`ControlCatalog.cpp` + `control_catalog.xml`（可用控件清单与默认属性）、`DesignerTools.h`、`UiDocUtil.h`、`samples\`、`res\`。
- **边界**：画布必须复用 MfcXmlCore 的绘制函数，不许自己算位置/另写一套渲染，否则与运行时不一致。

## 7. Viewer（Application → `Viewer.exe`，源码 `XmlViewer.cpp`）

**作用**：XML 预览与校验器（可当 CLI 用）。

- `XmlViewer.exe <file.xml>`：解析 + 语义校验 + 开窗渲染；`--check`：只校验并打印行号 + 节点路径；`--catalog`：指定控件目录。
- 校验内容：MSXML 语法错误、未知标签、枚举/数值/颜色/布尔格式、Name 重复、布局越界。
- 布局严格按 XML 的 Left/Top/Width/Height/Margin/Padding/Spacing 与 Grid.Row/Column，程序不推算位置。
- `XmlViewerMfcHost.h` / `XmlViewerNativeHost.h`：两种宿主实现（MFC 控件宿主 / 原生自绘）。
- 目录 `XmlViewer\` 只剩 `obj\`：历史构建残留，不是工程。
- **边界**：预览器是"校验 + 渲染"，不提供编辑；编辑走 UIDesign。

## 8. MfcXmlRuntime（Python 包，非工程）

**作用**：复合控件运行机制（纯 Python）：`xmlload` 按 XML 建对象图、`Runtime` 注册表与插件扫描、`Composite`/`Child` 事件代理与生命周期。

- 文件：`__init__.py`（导出 `xmlload` / `Runtime` / `Composite` / `Child` / `import_plugin` / `available`）、`runtime.py`（全部实现）。
- 几何全在 XML：`Composite._build` 只调 `M.xmlLayout(path, w, h)` 拿每个 `x:Name` 的矩形，Python 侧没有像素常量；`move()` = 换外框重新问一遍 XML。
- 回调签名自适应：`(comp, name, payload)` / `(comp, name)` / `(name)` / `()`。
- **边界**：只做"组合 + 事件"，不画控件；控件绘制仍在 C++。

## 9. plugins（XML 复合控件库，非工程）

**作用**：一个 `.xml` 一个复合控件（根 `<XWindow>` = 类型名，`x:Name` = 子控件名），加载时自动生成同名 `.py` 薄壳。

- 现有：`side_panel.xml`（Title / Table / BtnApply / BtnLoad / BtnClose，XML 内声明 `on_click="on_apply"` 等）。
- 下沉规则（满足任一即改 C++ 控件类）：内部有真实状态机、需要自绘、两处以上复用、进入数据热路径。
- 下沉时标签名/属性/回调名不变，Python 侧代码不用改 —— "XML 先跑通 → 需要时下沉"是可逆路径。
- **边界**：能摆位置就别写 Python 拼界面；状态机一旦出现说明该下沉。

## 10. common（共享头文件，非工程）

**作用**：跨工程共用的小头文件。`logPath.h`（日志路径）、`PyCommandFlags.h` / `PyDispatchFlags.h`（命令与派发标志位）、`PyString.h`（字符串转换）、`PyArxGuiHostC.h`（`PyArxGuiHost.dll` 的 C 接口约定：wxApp/ADUI palette 常驻宿主 + 可热替换 pyd）。

- **边界**：`PyArxGuiHostC.h` 是**外部 wx 宿主时代**的接口约定，当前主链路是 `PyMfcXml.pyd` + MFC 宿主；引用前先确认对应 dll 是否还在产物目录。

## 11. pylibs（内置第三方 Python 包，非工程）

**作用**：随 CAD 内嵌 Python 一起装载的依赖，免去现场 pip。现有：`numpy`、`requests`、`certifi`、`charset_normalizer`、`idna`、`urllib3`、`six`、`debugpy`（调试）。

- 由 PyArxEngine 初始化时挂到 `sys.path`；`Debug` 子模块靠 `debugpy` 做 attach。

## 12. tests（回归脚本，非工程）

**作用**：不依赖 UI 的自动化回归，按域分三组：

- `tests\cad`（24 个）：PyArxCore 的 CAD 侧脚本（在 acad 里跑）。
- `tests\mfc_controls`（26 个）：基础控件行为/渲染。
- `tests\mfc_xml`（50 个）：XML 布局、交互、插件、配电图（`pp_test_all.py` 27 项、`pp_split_test.py` 13 项、`pp_cell_test.py` 15 项等）。
- **约定**：跑 XML 渲染回归时设 `$env:PYARX_D2D_RT='software'`（软件光栅，避免显卡差异）。

## 13. tools（辅助工程与脚本）

| 子目录/文件 | 类型 | 作用 |
|---|---|---|
| `tools\cadmcp` | Python | 自建 MCP 服务器（stdio）：让外部 AI 在运行中的 acad 里执行 PyArx 代码、检索 API、读用法。`server.py` + `bridge.py` + `cad_send.ps1` |
| `tools\ShxProbe` | C++ → `ShxProbe.pyd` | SHX 文字矢量化探针：`AcGiTextEngine::tessellate` 封装（`explodeTexts` / `tessellate`），附探针脚本与样图 |
| `tools\Ttf2Line` | C++ → `Ttf2Line.pyd` | TrueType 文字转线条：GDI `GetGlyphOutlineW(GGO_NATIVE)` + 轮廓追踪（引擎不支持 TTF） |
| `tools\Text2Line` | 脚本 | 文字→线条的端到端脚本与探测（`explode_text2.py`、`glyph_trace.py`、`pyarx_make2.py`） |
| `tools\shxtext` | 脚本 | SHX 字体解码/取样 |
| `tools\hotbuild` | PS1 | 热构建：只重编改动工程并替换产物，配合 CAD 内 `pyreload` |
| `tools\*.py` / `*.ps1` | 脚本 | 维护：`declsplit.py`（类体切分）、`migrate_*.py`（标签/前缀迁移）、`gen_control_catalog.py`、`gen_control_doc.py`、`check_*.py`、`publish.ps1`、`git_tag_version.ps1` |

**边界**：`ShxProbe/Ttf2Line` 是可独立加载的 pyd，不进主解决方案；`tools\` 下的 `_` 前缀脚本是临时探针，不要当正式入口依赖。

## 14. 其他顶层目录

- `res\`：运行时资源（图标、光标、皮肤），**不随构建自动拷贝**，改完要手工复制到 `X64\Release`。
- `X64\Release\`：构建产物（`PyArxCore.pyd`、`PyMfcXml.pyd`、`MfcXmlCore.dll`、`PyArxEngine.arx`、`ShxProbe.pyd`、`Ttf2Line.pyd` 等）与 `python.exe`。不入库。
- `third_party\`：pybind11 / python315t / boost186 / zlib，不入库。
- `examples\`：示例工程。
- `docs\`：设计文档、评审结论、`pyarx_api_raw.json`（6.6 MB 全量 API 清单）、`PYARX_USAGE.md`（实测用法）。
- `log\`：运行日志（`pyengine.log`、`rawdb.log` 等）。

## 15. 改动落点速查

| 要改的东西 | 落到哪 |
|---|---|
| CAD 数据/几何 API 缺失 | `PyArxCore\PyDb*/PyGe*/PyEd*` + `PyArxBinder.cpp` 注册 |
| 新控件（有状态机/自绘） | `MfcXmlCore\controls\` 新建类 + `RegisterPyMfcControls.cpp` |
| 复合布局/组合 | `plugins\*.xml`（+ 必要时 `MfcXmlRuntime`） |
| 控件事件契约 | `MfcXmlCore\controls\XmlEvents.h` + 宿主派发 |
| 控件属性表 | `MfcXmlCore\controls\XmlTypes.h` |
| Python 侧 UI 接口 | `PyMfcXml\PyMfcBind.cpp`（唯一模块定义） |
| 窗口外壳/无框行为 | `MfcXmlFrame\XFrameWnd.*` |
| 设计器画布/属性面板 | `UIDesign\DesignerPane*`、`DesignCanvas.h`、`control_catalog.xml` |
| 预览校验规则 | `Viewer\XmlViewer.cpp` |
| CAD 命令/引擎初始化 | `PyArxEngine\acrxEntryPoint.cpp`、`PyEngine.cpp` |
| 让 AI 直接操作 CAD | `tools\cadmcp` |

## 16. 构建

```powershell
MSBuild PyArx.sln /t:PyMfcXml /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false
```

- 只改控件库：`/t:MfcXmlCore`；改 Python 接口：`/t:MfcXmlCore;PyMfcXml`（`PyMfcXml` 引用 `MfcXmlCore`）。
- 设计器/预览器：`/t:UIDesign;Viewer`。
- `PyArxCore`/`PyArxEngine` 需 ObjectArx SDK；`PyArxEngine` 装进 CAD 后重编译要先卸载 ARX。
