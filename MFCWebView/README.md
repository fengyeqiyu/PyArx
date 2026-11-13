# MFCWebView

MFC 只当窗口外壳，**窗体与侧栏里的控件全部由 Web（HTML/JS）渲染**；C++ 只提供页面拿不到的原生能力。

- 外壳（MFC）：窗口创建、消息循环、DPI、窗口状态
- 界面（Web）：侧栏、工具条、表单、表格、弹层、图标——全部 HTML
- 桥（Bridge）：`chrome.webview.postMessage` ↔ `WebView2::WebMessageReceived`

与 `MfcXaml`（XAML + D2D）完全独立、互不引用；`UIDesign` 的设计器画布保持不动。

## 目录

```
MFCWebView\            静态库（MFCWebView.lib）
  MFCWebView.h         统一入口
  WebViewHost.h/.cpp   WebView2 宿主（Pimpl，对外不暴露 COM）
  WebShell.h/.cpp      客户区铺满 + JSON 命令路由 + 原生能力
MFCWebViewDemo\        演示程序（MFCWebViewDemo.exe，双 WebView 实例）
  DemoApp.cpp          MFC 外壳（CFrameWnd），两栏布局 + 跨窗消息转发
  sidebar.html         侧栏（独立 WebView 实例）
  demo.html            主区（另一个 WebView 实例）
third_party\webview2\
  include\WebView2.h                 SDK 1.0.4314.35
  lib\x64\WebView2Loader.dll.lib
  bin\x64\WebView2Loader.dll         随 exe 分发
```

## 一个窗口两个 WebView（侧栏 + 主区）

同一个 MFC 窗口里开两个 `WebShell`，各挂一个子窗即可：WebView2 允许同进程多实例，
共享同一份 user data 目录。MFC 只负责两栏摆位与跨窗转发。

```cpp
// 侧栏：控件 ID 1000 的子窗；主区：控件 ID 1001 的子窗
m_side.attach(m_sideHost, L"", err);  m_side.loadPage(L"sidebar.html");
m_main.attach(m_mainHost, L"", err);  m_main.loadPage(L"demo.html");

// 页面发上来的自定义命令先交给应用层，返回 true 表示已接管
m_side.setCommandHandler([this](const std::wstring& json) { return onSideCommand(json); });
```

演示程序的跨窗协议：侧栏 `nav{page}` → MFC 转成 `nav.change{page,title}` 推给主区；
任一侧 `pane.toggle` → MFC 切显示状态并给两窗广播 `pane.state{collapsed,width}`；
侧栏右缘 `pane.dragbegin` → MFC `SetCapture` 拖宽（180–460 px）。

## 在 PyArxCore 侧栏里用（CAD 侧）

`PyArxCore` 的 `Ap.loadPalette(html, name, w, h)` 已经改走这条链路：
`CAdUiPaletteSet` 仍由 PyArxCore 建，页窗口的客户区交给 `MFCWebView` 渲染 HTML，
不再 import `PyMfcCore`、不再解析 XAML。

- 运行时依赖：`WebView2Loader.dll` 要随 `PyArxCore.pyd` 落在同一目录（构建后自动拷 `X64\Release\`）
- user data 目录：`%LOCALAPPDATA%\PyArx\WebView2Data`（AutoCAD 装在 Program Files 下，exe 同级不可写）
- 页面推消息用 `chrome.webview.postMessage`；Python 侧可用 `Ap.paletteEmit(name, json)` / `paletteEval(name, js)`

## 接入（三行）

```cpp
#include "MFCWebView.h"
webmfc::WebShell m_shell;

// OnCreate 里：把当前窗口的客户区整块交给 Web
m_shell.attach(m_hWnd, L"", err);          // err 为 std::wstring
m_shell.loadPage(L"ui\\index.html");       // 单文件 HTML
```

侧栏场景：`attach` 传侧栏那个子窗的 HWND 即可，库会自己跟着 `WM_SIZE` 铺满。
（侧栏要独立实例时，就是**单独再 new 一个 `WebShell`**，别用一张大页面里 `aside` 分栏。）

## 页面 → C++（`chrome.webview.postMessage({cmd:...})`）

| cmd | 参数 | 说明 |
|---|---|---|
| `shell.info` | — | 索取 DPI / 主题 / 客户区尺寸 / 窗口状态 |
| `window.title` | `text` | 改窗口标题 |
| `window.drag` | — | 自绘标题条拖动（内部 `WM_NCLBUTTONDOWN/HTCAPTION`） |
| `window.min` / `max` / `restore` / `close` | — | 窗口状态 |
| `window.resize` | `w`,`h` | 客户区尺寸（参数可平铺，也可放 `args` 下） |
| `window.center` | — | 居中到当前显示器 |
| `dialog.open` | `id`,`title`,`filter`,`name` | 原生打开对话框（`filter` 写 `XAML|*.xaml`） |
| `dialog.save` | 同上 | 原生保存对话框 |
| `dialog.message` | `id`,`text`,`title`,`level` | 原生 MessageBox（info/warn/error） |
| `clipboard.set` | `text` | 写剪贴板 |
| `clipboard.get` | `id` | 读剪贴板 |
| `shell.log` | `text` | 输出到 OutputDebugString |

## C++ → 页面（`chrome.webview.addEventListener('message')`）

| event | 载荷 |
|---|---|
| `shell.ready` | `dpi`(真实 DPI，如 96),`scale`(缩放比),`theme`,`width`,`height`,`state` |
| `shell.info` | 同上 |
| `window.resized` | `width`,`height`,`state` |
| `window.state` | `state`,`width`,`height` |
| `dialog.open` / `dialog.save` | `id`,`canceled`,`path` |
| `dialog.message` | `id` |
| `clipboard.set` | `ok` |
| `clipboard.get` | `id`,`text` |
| `shell.error` | `text` |

`state` 取值：`normal` / `max` / `min` / `hidden`。

## 约定

- **坐标即 DIP**：页面 CSS px = DIP。原生对话框、窗口尺寸用的都是像素，跨 DPI 时由库按 `GetDpiForWindow/96` 折算。
- **右键菜单归页面**：已关掉 WebView2 默认菜单（`AreDefaultContextMenusEnabled=FALSE`），要在页面里自己画。
- **开发者工具保留**：调试期可 `F12` 或右键调试；发布时可关。
- **本地页面走 `file://`**：单文件 HTML 直接 `loadPage`；需要多文件/相对资源时改为虚拟主机映射（后续加）。

## 构建

```
MSBuild MFCWebView\MFCWebView.vcxproj        /p:Configuration=Release /p:Platform=x64
MSBuild MFCWebViewDemo\MFCWebViewDemo.vcxproj /p:Configuration=Release /p:Platform=x64
```

输出统一落 `G:\PyArx\X64\Release\`：`MFCWebView.lib`、`MFCWebViewDemo.exe`、`demo.html`、`WebView2Loader.dll`。

## 已知边界

- 只支持 x64 Release（与 `PyArx.sln` 一致）。
- `WebView2Loader.dll` 必须与 exe 同目录（PostBuild 已自动拷贝）。
- 依赖本机已安装的 WebView2 运行时（本机 `157.0.4341.42`）。
- 页内输入法、拖选、快捷键由 Chromium 负责，不会和 MFC 加速键冲突；`Ctrl+S` 这类要自己 `keydown` 里发命令。
- 调试注入鼠标消息要投到 **Chromium 的 `Chrome_WidgetWin_*` 子窗**（且是子树里含 `Chrome_RenderWidgetHostHWND` 的那个），投主窗无效。
