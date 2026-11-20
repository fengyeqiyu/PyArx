# PyArx 项目结构（2026-11-15 整理）

## 核心（保留，勿动）

| 目录 | 作用 |
|---|---|
| `PyArxCore` | C++ / pybind11 核心，ObjectArx 封装（`handle()` / `ownerId()` / 事务 / 块表） |
| `PyArxEngine` | 引擎源码，产物 `X64\Release\PyArxEngine.crx` |
| `MfcXmlCore` / `MfcXmlFrame` / `MfcXmlRuntime` / `PyMfcXml` | MFC + XAML 体系 |
| `pyarx_vision` | 扫描与识别（`scan.py`、detect、build_plan、regress） |
| `examples\cad2d` | **看图主线**：`webview.py`（scan → scene JSON）、`webview_template.html`（Canvas2D 渲染器）、`scan_load.py` |
| `X64\Release` | 构建产物：python 3.15t（free-threaded）、PyArxCore.pyd、PyArxEngine.arx/crx |
| `ACAD` | accoreconsole 完整镜像（5199 文件 / 2896 MB），必须整目录，裁剪即崩 |
| `third_party` / `pylibs` | 第三方依赖 |
| `tools` | `cadmcp`、`hotbuild`、`shxtext`、`Text2Line` 等 |
| `plugins` / `res` / `UIDesign` | XAML 复合控件与资源 |
| `tests` | 测试与交付物；3 张成品 HTML 在根下 |
| `docs_notes` | 从旧 docs 抢救出的 15 份设计/接口文档（md） |

## 归档（`_attic\20261115`，共 306 MB，确认无误后可整目录删）

- `CAD快速看图` — 第三方看图软件（125 MB，外部工具，非本项目代码）
- `XmlViewer` — 只剩 `obj\` 中间产物（37.5 MB）
- `Viewer` — 早期 XML 查看器试验工程
- `PyArxDwg` — 纯 Python 解 2024 DWG，已验证做不通（`FileDepList` 加密）
- `log` — 构建日志与一次性探针
- `docs` — 历史调试截图堆（101 MB，911 文件）
- `tests_shots` — 历史截图、truth 校验 json、dxf

## 主入口

```
cd G:\PyArx\examples\cad2d
G:\PyArx\X64\Release\python.exe webview.py "<dwg>" -o "<html>"
```

交互：滚轮缩放（跟光标）、拖动平移、双击 / `0` 回全图；底栏可切 `视图1..5`/`全图`、`图层`、`框选导出`；单击图元 = 点选，可隐藏此图元或整层。

## viewer：C++ + WebView2 应用（已可运行）

| 路径 | 作用 |
|---|---|
| `viewer\PyArxViewer.exe` | 174 KB 外壳（WebView2 静态链接，无额外 dll） |
| `viewer\src\main.cpp` | 窗口、WebView2 控制器、打开/重载消息通道、深色标题栏 |
| `viewer\web\index.html` | 由 `make_web.py` 从模板生成，scene 数据外置 |
| `viewer\web\scene.json` | 默认载入的示例（101 中试车间） |
| `viewer\make_web.py` | 模板 → 外置数据版 index.html |
| `viewer\build.ps1` | 一键编译（vcvars64 + cl） |
| `viewer\shot.py` | 抓窗口 / 整屏截图（不依赖 PIL） |
| `viewer\_sdk\wv2` | WebView2 SDK（include + WebView2LoaderStatic.lib） |

用法：双击 `PyArxViewer.exe`（`web` 目录必须与 exe 同级），底栏「打开 JSON」换图。
数据产出：`python webview.py "<dwg>" -j -o "<name>.scene.json"`。

要点：web 端零改动复用同一套 Canvas2D 渲染器；C++ 只负责窗口、虚拟主机映射
（`https://app.local/` → `web\`）、文件选择与 `postMessage` 通道。

**导出落盘**：外壳用 `DownloadStarting` 接管下载，PNG 固定存到 `viewer\export\`（同名覆盖），
不再依赖 WebView2 默认下载路径（默认路径不可写时会静默丢文件）。

**清晰度取决于「选区世界尺寸 ÷ 输出像素」**，面板会显示「相对当前视图 N 倍」。核对单台柜
建议 4800 档、选区只框一台柜；框太大时文字只剩 1~2px，等于白导。

