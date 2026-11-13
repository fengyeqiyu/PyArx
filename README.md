# PyArx

AutoCAD（ObjectArx）+ MFC/XAML 的 Python 绑定与控件库。C++ 侧用 pybind11 暴露成 Python 模块，界面层用 MFC 承载一套自绘控件库，XAML 只做声明式布局与事件绑定。

## 目录

| 目录 | 内容 |
|---|---|
| `PyArxCore` | ObjectArx 核心封装（Python 模块入口、CAD 交互） |
| `PyArxEngine` | 几何/业务引擎 |
| `MfcXamlCore` | XAML 解析、控件库（`controls\Xaml*.h/.cpp`）、属性表 `XamlTypes.h`、事件契约 `XamlEvents.h` |
| `PyMfcXaml` | MFC 宿主（窗口、皮肤自绘、事件派发，`PyMfcHost` / `PyMfcProcs` / `PyMfcLoad`） |
| `PyMfcWebView` / `MFCWebView` | WebView 承载 |
| `PyArxQml` / `UIDesign` / `Viewer` / `XamlViewer` | QML 与设计器/预览工具 |
| `plugins` | 用 XAML 组合出来的复合控件（不应下沉到 C++ 的通用组合除外） |
| `tests` | 回归脚本（`tests\mfc_xaml` 下按控件/交互分类，均为可重复运行的模拟消息测试） |
| `docs` | 设计/评审/执行记录 |
| `res` | 运行时资源（图标、光标等） |

`third_party`（pybind11 / zlib / python314 等）与本机构建产物 `X64` 不入库，按需自行准备。

## 构建

```
MSBuild PyArx.sln /t:PyMfcXaml /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false
```

产物在 `X64\Release`（`PyMfcXaml.pyd`、`MfcXamlCore.dll`）。`res` 目录不随构建自动拷贝，改资源后需手工复制到产物目录。

## 设计约定

- 控件属性走 `XamlPropEntry` 属性表：名字、类型、只读/只写、读写实现只在一处定义，基类统一分派。
- 事件走 `XamlEvents.h` 的契约表：XAML 只写事件名，控件申报 `events()`，宿主按契约绑定并派发。
- 控件回调走 Python：XAML 里写 `on_click="fn"`，或加载后 `child.click = fn`。
- C++ 只提供基础控件；复合控件用 XAML 组合放在 `plugins`，确认通用后再下沉。

## 许可

见 `LICENSE`。
