# PyArx

AutoCAD（ObjectArx）+ MFC/XAML 的 Python 绑定与控件库。C++ 侧用 pybind11 暴露成 Python 模块，界面层用 MFC 承载一套自绘控件库，XAML 只做声明式布局与事件绑定。

## 项目简介

PyArx 是一个将 AutoCAD 的 ObjectArx 框架与 MFC/XAML UI 技术桥接到 Python 的开发库。通过 pybind11 实现 C++ 到 Python 的绑定，结合自绘 MFC 控件库和 XAML 声明式 UI，实现 CAD 二次开发中的现代化界面开发能力。

### 核心特性

- **Python 绑定**：使用 pybind11 将 ObjectArx 核心类库暴露为 Python 模块
- **自绘控件库**：基于 MFC 的完全自绘控件，支持皮肤定制
- **XAML 布局**：支持 XAML 声明式 UI 定义，配合 Python 事件回调

## 目录结构

| 目录 | 说明 |
|------|------|
| `PyArxCore` | ObjectArx 核心封装，包含 Python 模块入口、CAD 数据库交互、实体对象、事务管理等 |
| `PyArxEngine` | 几何计算与业务逻辑引擎 |
| `MfcXamlCore` | XAML 解析引擎、控件库（controls\Xaml*.h/.cpp）、属性表（XamlTypes.h）、事件契约（XamlEvents.h） |
| `PyMfcXaml` | MFC 宿主程序，包含窗口管理、皮肤自绘、事件派发（PyMfcHost / PyMfcProcs / PyMfcLoad） |
| `MfcXamlRuntime` | 复合控件运行框架（Python 实现） |
| `UIDesign` | UI 设计器 |
| `Viewer` / `XamlViewer` | 预览工具 |
| `plugins` | 用 XAML 组合的复合控件（通用组合下沉到 C++ 基础控件） |
| `tests` | 回归测试脚本（tests\mfc_xaml 下按控件/交互分类） |
| `docs` | 设计文档、评审记录、执行记录 |
| `res` | 运行时资源（图标、光标等） |

> **注意**：`third_party`（pybind11 / zlib / python314 等）与构建产物 `X64` 不入库，按需自行准备。

## 构建说明

### 环境要求

- Windows 64 位系统
- Visual Studio / MSBuild
- Python 3.14 或兼容版本
- ObjectArx SDK

### 构建命令

```bash
MSBuild PyArx.sln /t:PyMfcXaml /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false
```

### 构建产物

- 输出目录：`X64\Release`
- 主要产物：
  - `PyMfcXaml.pyd` - Python 可导入模块
  - `MfcXamlCore.dll` - 核心控件库

> **注意**：`res` 目录不随构建自动拷贝，修改资源后需手工复制到产物目录。

## 设计约定

### 属性系统

控件属性统一走 `XamlPropEntry` 属性表：
- 名字、类型、只读/只写属性
- 读写实现只在一处定义
- 基类统一分派机制

### 事件系统

事件走 `XamlEvents.h` 的契约表：
- XAML 端只写事件名
- 控件申报 `events()` 方法
- 宿主按契约绑定并派发

### 回调机制

- Python 回调：XAML 里写 `on_click="fn"`，或加载后 `child.click = fn`
- 控件事件通过契约表统一管理

### 控件层级

- **C++ 层**：只提供基础控件
- **XAML 层**：复合控件用 XAML 组合，放在 `plugins` 目录
- 确认通用后下沉到 C++ 层

## 核心模块说明

### PyArxCore

ObjectArx 核心 Python 绑定，包含：
- 数据库操作（PyDbDatabase、事务管理）
- 实体对象（线、圆、多段线、尺寸标注、块引用等）
- 符号表（图层、线型、块表等）
- 选择集与过滤
- 反应器（数据库反应器、对象反应器）
- 侧栏（CAdUiPaletteSet / CAdUiPalette），页内控件宿主由 GUI 层回填

### MfcXamlCore

XAML 解析与控件系统：
- XamlCore：XAML 解析、布局、样式、模板、绑定表达式
- 控件库：Button、TextBox、ListBox、TreeView、GridView、TableView 等
- 自绘皮肤：ProgressBar、Slider、Combo、Tab、Calendar、Menu 等
- 几何引擎：基于 Boost.Geometry 的几何计算
- SVG 支持：nanosvg 实现的 SVG 渲染
- D3D11 渲染：Direct3D 11 后端渲染

### MfcXamlRuntime

复合控件运行框架（Python 实现）：
- Composite：复合控件容器
- Runtime：控件运行时管理
- 自动事件绑定与属性映射

## 版本与分支约定

- `master` 永远是最新代码，所有提交直接落在 `master` 上；历史版本不靠分支保存，靠 **tag** 固定。
- 每个提交对应一个 `v主.次.补丁` 标签，标签信息即提交主题，随时 `git checkout v0.9.0` 或 `git show v0.9.0` 回看那一版。
- 递增规则（由脚本自动执行）：
  - 提交主题以 `feat` / `refactor` 开头，或属于控件封装批次 → 次版本 +1、补丁归零
  - 其余（`fix` / `chore` / `build` / `docs` / `test`、网页端改 README 等）→ 补丁 +1
  - 主题含 `BREAKING` → 主版本 +1、次版本与补丁归零
- 打标签：`.\tools\git_tag_version.ps1`（`-DryRun` 只看版本号，`-Push` 打标并推送，需 `$env:GITEE_TOKEN`）。

## 许可

见 `LICENSE` 文件。
