# MfcXml 项目结构（整理后）

## 1. 分层

| 层 | 目录 | 职责 | 依赖方向 |
|---|---|---|---|
| 解析/样式/布局引擎 | `MfcXmlCore/` | XML → 元素树、样式求值、布局矩形、控件注册表 | 只依赖 Win32 + std，不认识 Python，不依赖 MFC |
| 控件层 | `MfcXmlCore/controls/` | 具体控件实现（`XmlMfcControlBase` + `XmlFlexPanel` 等） | 依赖 `XmlCore.h` 与 `XmlMfcHost.h` |
| 渲染 | `MfcXmlCore/XmlD2D.h` `XmlSurface` `XmlSvgScene` `XmlGeometry` | D2D/GDI 两套绘制后端 | 依赖控件层 |
| MFC 宿主 | `PyMfcXml/` | `PyMfcHost`（MFC 窗口、消息、子类化、自绘、控件工厂） | 依赖 `MfcXmlCore` |
| Python 绑定 | `PyMfcXml/PyMfcBind.cpp` `XmlBindings.cpp` | pybind11 模块 `PyMfcXml`（`Window`/`Pane`/`xml`/`pp`） | 依赖宿主层 |

## 2. PyMfcXml 实现拆分（原 9447 行单文件 → 10 个 TU）

- `PyMfcHost.Base.cpp`：公共前置 + 文件级辅助（hover 标记、拖拽条、剪贴板、边框像素码等）
- `PyMfcHost.HostBase.cpp`：生命周期、主题、字体、皮肤盒、id 缓存
- `PyMfcHost.Procs.cpp`：子类化窗口过程（tab / dragbar / owner-draw / splitter / 事件转发）
- `PyMfcHost.Paint.cpp`：自绘节点、命中测试、鼠标态、容器背板
- `PyMfcHost.Layout.cpp`：attachXml、reflowXml、容器重排、层级/z 序
- `PyMfcHost.Controls.cpp`：建控件、表格/树/工具条取值、窗口外观
- `PyMfcHost.Dwg.cpp`：DWG 画布、3D 视口、Ribbon、FileTab、图片
- `PyMfcHost.Anim.cpp`：动画、flickable、翻页滑动
- `PyMfcHost.Interop.cpp`：Python 回调登记与事件派发
- `PyMfcHostInternal.h`：跨 TU 的文件级辅助声明（非公开接口，命名保持原样不做 `pmh` 包裹，避免与 `PyMfcHost.h` 里既有全局声明冲突）

`PyMfcHost.cpp` 仅留一份目录说明，不再承载实现。

## 3. MfcXmlCore/XmlCore 拆分（原 7540 行单文件 → 11 个 TU + 2 个头）

`XmlCore.cpp` 只留一段说明。实现按节拆成 11 个编译单元，全部 `PrecompiledHeader=NotUsing`：

| TU | 行数 | 内容 |
|---|---|---|
| `XmlCore.Element.cpp` | 96 | `XmlElement` |
| `XmlCore.Util.cpp` | 1717 | 属性读写、长度/颜色解析、单位换算、`ParseElement` 等内部工具 |
| `XmlCore.Text.cpp` | 110 | 文本基线度量 |
| `XmlCore.Layout.cpp` | 1259 | CSS flex（含 wrap/align-content）、CSS Grid（轨道/auto-placement/项默认拉伸）、绝对定位、`inset:0` 撑满父 |
| `XmlCore.Api.cpp` | 194 | `XML_CORE_API` 导出面（布局/命中/自然尺寸/默认尺寸） |
| `XmlCore.Html.cpp` | 2384 | `htmlsugar` 命名空间：HTML 词汇糖、别名表、控件族映射 |
| `XmlCore.Registry.cpp` | 52 | `XmlControlRegistry` |
| `XmlCore.Runtime.cpp` | 376 | `XmlRuntime`（样式求值 / 模板 / 绑定） |
| `XmlCore.Style.cpp` | 414 | 样式表、内联 style、`applyStyles`、delegate 展开 |
| `XmlCore.Expr.cpp` | 688 | 内置表达式（`Bind`/`If`/格式化） |
| `XmlCore.Schema.cpp` | 108 | 属性 / 事件 schema |

两个头：`XmlCoreCommon.h`（原 1-21 行公共前置，所有 TU 共享）、`XmlCoreInternal.h`（跨 TU 声明：`xmlcore_impl` 里的共享函数声明 + `XmlFlow`/`LayoutDefaults`/`TemplateTable` 类型定义 + 全局作用域的 `AttachPanel`/`ParseElement` 等原型，末尾一条 `using namespace xmlcore_impl;` 让各 TU 保留原裸名写法）。

跨 TU 的做法：原先横跨工具/文本/布局三节的大匿名 namespace 改名 `xmlcore_impl`（外部链接），其余各节匿名 namespace 一并改名；共享件定义留在原 TU，只在 `XmlCoreInternal.h` 放声明。生成器 `tools/gen_xmlcore_internal.py` 从定义处反推声明（去 `static`/`inline`、去默认实参、按 `masked` 文本区分命名空间归属），`tools/add_decls.py` 按编译错误增量补声明，`tools/split_xmlcore.py` / `tools/split_engine.py` 是拆分脚本。

## 4. Python 接口命名

Python 侧统一 snake_case（245 个名字，468 处 `.def`）。C++ 成员名仍是 camelCase（`PyMfcHost::addButton` ← `.def("add_button", &PyMfcWindow::addButton)`），改名映射表见 `log/bind_rename_map.json`，仓库内 `.py` 调用点已同步（273 个文件）。

模块级函数：`guiload` / `guiload_text` / `xml_layout` / `prop_schema` / `event_schema` / `control_events` / `event_warnings` / `prop_warnings` / `clear_diagnostics` / `register_control` / `create_embedded` / `pump` / `shutdown` / `easing_value`。

## 5. 已知待办（下一轮）

1. ~~`Window` / `Pane` 两套绑定重复~~ 已完成（第 85 轮）：`PyMfcBind.cpp` 改一张模板 `RegisterSurfaceApi<C>(cls)` 登记共用 221 个方法，两侧只留专属（win 30 / pan 6）；生成器 `tools/dedupe_bind.py`。
2. ~~`MfcXmlCore/XmlCore.cpp`（7540 行）按节拆分~~ 已完成（第 87 轮）：拆成 11 个 TU + `XmlCoreCommon.h` / `XmlCoreInternal.h`。
3. ~~QML 专有残留（`controls/XmlQmlNames.h`、`anchors.*`、Dock）删除~~ 已完成（第 88 轮）：`XmlQmlNames.h` → `controls/XmlPropAliases.h`；`anchors.*` 属性面与兄弟/父部分边锚定、`Dock` 切分块全部删除，只留 CSS 语义的 `fillParent` / `fillMargin`（`inset:0` 的内部实现）；样例 08/09/10/15 与 `t_resize` 改写成 CSS 写法。
4. 仍是 QML/XAML 混写的残留（下轮）：`UIDesign/samples` 里 04/05/06/11/12/13/14 用 `Layout.*`、`Orientation`、`Spacing`；`XmlCore.Util.cpp` 仍解析 `Layout.alignment` / `Layout.fillWidth` / `fillHeight`（样例在用的历史通道，迁到 `flex:1` 前不能断）；样例文件名仍带 `_qml_`。
6. ~~CSS Grid 只在 XAML 通道可用~~ 已完成（第 89/90 轮）：`display:grid` + `grid-template-columns/rows` + `grid-column/grid-row`（含 `1fr`、`repeat()`、`span`）落成 `XGrid`，auto-placement 按 `grid-auto-flow:row`；网格项默认两轴拉伸，原生 `<button>/<input>` 的伪尺寸改用跨重算保留的 `sugarW/sugarH` 标记，不再撑住格子。
7. ~~`box-shadow` 写成"架构性缺失"~~ 已补（第 94 轮）：外投影改由宿主整窗画布逐圈落笔（`XmlDraw.h::xmlDrawBoxShadow`），`inset` 仍走容器位图刷；`t_box_shadow` / `t_shadow_list` 全绿。
8. 仍是 QML/XAML 混写的残留（下轮）：`UIDesign/samples` 里 01–05、11–15 用 `Layout.*`、`Orientation`、`Spacing`；样例文件名仍带 `_qml_`。5. `xml_view.py` 手测路径与 `win.onResize` 重生成 `<XScene>` 的写法复核。

## 6. 维护约定

- 源文件是 UTF-8 **带 BOM**；改写必须用 `.NET UTF8Encoding(true)`，否则双 BOM 触发 `C2014/C4430`。
- 构建只跑 `/m:1`（并行编译有 obj 竞态）。
- 每轮改动必须 `EXIT=0` + 受影响套件全绿才计入台账。
