# PyArx 三套界面方案对比表

> 目的：回答"Python 侧怎么控制界面"这个问题时，把工程里已有的 MFCXaml、MFCWebView 和当前在做的 QML 放在同一张表上比。
> 所有结论均来自源码实地核对，行号对应当前仓库状态（2026-11-10）。

## 0. 三套方案在哪

| 方案 | 外壳 | 界面渲染 | Python 绑定源码 | pyd 状态 |
|---|---|---|---|---|
| MFCXaml | MFC + D2D 自绘 | 自研 XAML 解析器（非 WPF） | `MFCXaml 此文件夹项目暂停修改\PyMfcXaml\PyMfcXaml.cpp:9151`、`XamlBindings.cpp` | 只在 `X64\_backup_20261109\PyMfcXaml.pyd`，**项目暂停** |
| MFCWebView | MFC 窗口壳 | WebView2 / Chromium，HTML+JS | `PyMfcWebView\PyMfcWebView.cpp:459` | `X64\Release\PyMfcWebView.pyd` 在 |
| QML | 宿主 HWND（CAD 或自建） | Qt Quick 6.2.6 | `PyArxQml\PyArxQmlModule.cpp` | `X64\Release\PyArxQml.pyd` 在（今天新版，含 tree/set_path） |

## 1. 总体能力对比

| 维度 | MFCXaml | MFCWebView | QML（当前） |
|---|---|---|---|
| 界面描述语言 | 自研 XAML（单趟布局，无 measure/arrange、无 `{Binding}`、无 DataTemplate） | HTML + CSS + JS，标准生态 | QML（声明式 + JS 表达式 + 属性绑定） |
| 控件来源 | 内建控件表 + `RegisterPyMfcControls` 自定义控件 | 页面自己画，C++ 只提供原生能力 | Qt Quick 内建 + `qmlRegisterType`（尚未用） |
| **寻址方式** | 运行时控件：数字 `id`；XAML 元素：`Name="..."` | 无寻址概念，JS 里自己 `querySelector` / `getElementById` | `objectName` / 树路径 `"0/5/0/1"` / 按内容反查 |
| **改值** | 逐类型方法 `setText(id,s)`、`setCheck`、`setCurSel`、`setPos`、`setRange`…（100+ 个）；通用 `setXamlValue(name,prop,value)` | `eval("...")` 一条 JS 表达式通吃 | `set_prop(name,prop,v)` / `set_path(path,prop,v)` / `set_named` |
| **读值** | `getText` / `getCurSel` / `getCheck` / `getPos` / `getVisible`… | ❌ `ExecuteScript(js, nullptr)` 不接回调，`eval` **拿不到返回值**；只能靠页面 `postMessage` 推回 | ✅ `get_prop` / `get_path` 直接返回 |
| **调界面里的函数** | ❌ 无通用入口；只能预注册 `onClick/onChange/onSelect/onChecked` 回调 | 间接：发命令给页面，页面自己执行；`setCommandHandler` 收结果 | ✅ `call/ call_named / call_path`，直接拿返回值（0~4 参） |
| 值类型 | 全部 `std::wstring`，数字布尔列表要自己字符串化 | 全字符串 / JSON 字符串 | `QVariant` ↔ int/float/bool/str/list/dict 直通 |
| 刷新机制 | 显式 `reflowXaml()` 重新布局 | 浏览器自身重排重绘 | 属性绑定自动传播（改根属性即可） |
| 对象树导出 | ✅ `Document.root/children/find/findTag/names()/toDict()/bounds/gridPos` | ❌ 无；树在 DOM 里，自己 JS 遍历 | ✅ `tree(max_depth, max_nodes)` 导出 path/class/type/objectName/text/x/y/w/h/visible |
| 新增一个控件的 C++ 成本 | 高：要么写逐类型方法，要么靠 `setXamlValue` 走字符串 | **零**：页面加个 div 就行 | **零**：QML 加个元素 + 可选 objectName |
| 名称稳定性 | 强：`Name` 写在标记里，文件头还列出"改名会导致画布不出图"的清单 | 强：HTML id 天然稳定 | 弱：`id` 不进元对象系统，`findChild` 找不到；需显式 `objectName` 或走路径 |
| 样式/动画能力 | 自绘，需自己写绘制分支 | CSS 全集，最强 | QML `Behavior`/`Transition`/`ShaderEffect`，强 |
| 调试手段 | 输出日志 | F12 Chromium DevTools | 有限的 qml 调试；建议 `console.log` + `tree()` |
| 外部依赖 | 无 | `WebView2Loader.dll` 必须同目录 + 本机 WebView2 运行时（本机 157.0.4341.42） | Qt 6.2.6 的 Core/Gui/Qml/Quick DLL + `qml\` 目录 + 平台插件 |
| CAD 内部署 | 已验证可用 | 已验证可用（`Ap.loadPalette` 已改走这条链路） | 已验证可用，但 `QGuiApplication` 构造在 CAD 进程内要 3.2s |
| 现状 | 暂停修改 | 在用 | 提速已完成，Python 双向控制已交付 |

## 2. 同一需求的三种写法

### 需求 A：把一个显示文字改成 6912

```python
# MFCXaml —— 数字 id（逐个控件方法）
w.setText(1002, "6912")

# MFCXaml —— 按 XAML Name 走通用属性
w.setXamlValue("resultText", "Text", "6912")
w.reflowXaml()          # 改完要显式重排

# MFCWebView —— 一条 JS
w.eval("document.getElementById('resultText').textContent = '6912'")

# QML —— objectName 或树路径
panel.set_named("resultText", "text", "6912")
panel.set_path("0/5/0/1", "text", "6912")
```

### 需求 B：读一个输入框的值

```python
w.getText(1005)                                          # MFCXaml：直接返回
# MFCWebView：拿不到。只能 w.emit('{"ask":"value"}')，页面 postMessage 回推
panel.get_named("inputBox", "text")                      # QML：直接返回
```

### 需求 C：往表格/列表里追加一行

```python
w.addRow(1010, ["A", "B"])                               # MFCXaml：有专门方法
w.eval("addRow('A','B')")                                # MFCWebView：页面里自己写的函数
panel.call("addRow", ["A", "B"])                         # QML：直接调 QML 函数并取返回值
```

### 需求 D：界面按钮被点 → 通知 Python

```python
w.onClick(1001, lambda: print("clicked"))                # MFCXaml：预注册回调
w.setCommandHandler(lambda js: (print(js), True)[1])     # MFCWebView：页面发 postMessage
# QML：QML 侧调用注入的 backend 对象（QmlBackend 的 Q_INVOKABLE）
```

### 需求 E：调界面里的纯函数并把结果算回 Python

```python
# MFCXaml：无此能力
# MFCWebView：间接，页面执行后 postMessage 回推
panel.call("calcTotal", 12)      # QML：一次调用，直接拿到返回值
```

## 3. 性能与开销（实测 vs 未测）

| 项 | MFCXaml | MFCWebView | QML |
|---|---|---|---|
| 首次建界面耗时 | **未测** | **未测**（历史经验：WebView2 异步初始化有"首屏晚半拍"现象） | 实测：python 独立进程总 452~484ms（import 10~14 / ensure 280~300 / attach 123~135 / 首帧 47~48） |
| 宿主进程内额外代价 | — | 未知 | **CAD 内 `QGuiApplication` 构造 3168~3221ms**（一次性） |
| 常驻内存 | 最低（自绘） | 高（一个 Chromium 进程树） | 中（Qt Gui/Quick 约 20MB+ DLL） |
| 依赖分发 | 零 | `WebView2Loader.dll` + 运行时 | Qt DLL 15 个 + `qml\` 目录 + platform plugin |

> 注：QML 那 3.2s 只在 CAD 进程内发生，python 独立进程只有 0.28s；根因疑为 CAD 把插件路径劫持到 `C:/Program Files/Autodesk/AutoCAD 2024/plugins`，探针 `tests\qt_spike\pyarx_qml_probe_qpa.py` 尚未跑完。

## 4. 风险与已知边界

| 方案 | 已知风险 |
|---|---|
| MFCXaml | ① 项目已暂停，pyd 只在备份目录，重新启用需先编译；② XAML 语义与 WPF 不同（单趟布局、容器只有 5 种、Canvas 是流式容器），容易按 WPF 习惯写错；③ `Node` 是只读视图 + `keep_alive` 挂 `Document`，文档一析构 Node 失效 |
| MFCWebView | ① 只支持 x64 Release；② `WebView2Loader.dll` 必须与 exe/pyd 同目录；③ 依赖本机 WebView2 运行时版本；④ user data 目录 `%LOCALAPPDATA%\PyArx\WebView2Data` 单进程独占，同机再跑一个实例会静默失败；⑤ **`eval` 无返回值**，读界面只能回推；⑥ 调试注入鼠标消息要投到 `Chrome_WidgetWin_*` 子窗，投主窗无效 |
| QML | ① CAD 内 `QGuiApplication` 构造 3.2s 尚未消除；② Debug 配置未编（`/MDd` 需另编 Qt，约 40 分钟）；③ 依赖 Qt 版本严格对齐 6.2.6，CAD 升级到 6.4/6.5 会撞同名 DLL 且无版本闸；④ QML 的 `id` 不进元对象系统，必须靠 `objectName` 或树路径；⑤ `setProperty` 写**未声明**的动态属性不参与绑定、界面不刷新，必须在 QML 里声明 `property` |

## 5. Python 接口一览（可直接抄）

| 能力 | MFCXaml | MFCWebView | QML |
|---|---|---|---|
| 载入界面 | `loadFile(xaml)` / `PyMfcCore.xaml.load(path)` | `loadFile(html)` / `loadUrl(url)` / `webLoad(page,w,h,title,onCmd,onReady)` | `load(qml)` / `attach(hwnd)` |
| 按名改属性 | `setXamlValue(name,prop,val)` | `eval(js)` | `set_named(name,prop,v)` |
| 按 id 改属性 | `setText/setCheck/setCurSel/setPos/setRange/setVisible/move` | — | — |
| 按路径改属性 | —（无路径概念，靠 name） | — | `set_path("0/5/0/1",prop,v)` |
| 读值 | `getText/getCheck/getCurSel/getPos/getVisible/getEnabled` | ❌ | `get_prop/get_named/get_path` |
| 调界面函数 | ❌ | 间接（页面执行 + 回推） | `call/call_named/call_path` |
| 导对象树 | `Document.root/children/find/findTag/toDict/bounds/gridPos` | ❌ | `tree(max_depth,max_nodes)` |
| 全量名字字典 | `Document.names()` | ❌ | 待补：`tree()` 已含 objectName，聚合两行即可 |
| 推事件到界面 | 回调 + `reflowXaml` | `emit(json)` / `paletteEmit(name,json)` | 改属性即触发绑定 |
| 收界面事件 | `onClick/onChange/onSelect/onDoubleClick/onDropDown/onValue/onChecked` | `setCommandHandler(fn(json)->bool)` | QML 调注入的 `backend` 对象 |
| 一步到位入口 | — | `webLoad(...)` + `webRun` + `runFor(ms)` + `parseJson` | `pyarx_qml.QmlPanel` 代理全部方法 |
| 位置控制 | `move(id,x,y,w,h)`、`bounds` 可读 | 页面 CSS | `x/y/width/height` 属性 |

## 6. 结论

1. **"要不要给每个控件写封装" 这三套的答案都是不要。**
   - MFCWebView 证明得最彻底：只有 `eval` + `emit` 两个原语，什么都不用封装，代价是**没有结构化读写**。
   - QML 走元对象系统，同样是通用桥（加控件 C++ 一行不改），但比 WebView 多了"读值 + 取返回值 + 保类型"。
   - MFCXaml 是唯一"逐控件方法表"的方案，100+ 个方法，属于历史包袱式设计。

2. **名称可寻址性：XAML 最强，HTML 次之，QML 最弱。**
   - XAML `Name` 写在标记里、`names()` 一键全文档映射；
   - HTML id 天然稳定；
   - QML 的 `id` 不进元对象系统 —— 这是必须显式写 `objectName` 或用树路径的唯一原因。

3. **QML 侧唯一值得补的两件**：
   - `names()`：把 `tree()` 里的 `objectName` 聚合成 `{name: type}`，形状对齐 `PyMfcCore.xaml.Document.names()`；
   - `eval(expr)`：用 `QQmlExpression` 执行 JS 表达式并**取回值** —— 拿到 WebView 那种"一行字符串控全场"的便利，同时保留返回值（这是 WebView 做不到的）。

4. **选型建议**：要跨平台/动效/现代观感 → QML；要最强样式与生态、能接受 Chromium 体积 → WebView2；已有 XAML 资产且不追求新特性 → 维持 MFCXaml（但它当前是暂停状态）。

## 7. 相关文件

| 用途 | 路径 |
|---|---|
| QML 双向控制说明 | `docs\Python控制QML界面说明.md` |
| QML 提速记录 | `docs\QML面板提速优化记录.md` |
| MFCXaml 绑定源码 | `MFCXaml 此文件夹项目暂停修改\PyMfcXaml\PyMfcXaml.cpp`、`XamlBindings.cpp` |
| MFCXaml 命名清单 | `MFCXaml 此文件夹项目暂停修改\UIDesign\designer_layout.xaml` 文件头注释 |
| WebView 桥协议 | `MFCWebView\README.md`（命令表 / 事件表） |
| WebView 绑定源码 | `PyMfcWebView\PyMfcWebView.cpp:459-526` |
| WebView 底层 | `MFCWebView\WebViewHost.cpp:250/283/359/375` |
| CAD 侧栏包装 | `PyArxCore\PyPaletteSet.cpp:393-416` |
