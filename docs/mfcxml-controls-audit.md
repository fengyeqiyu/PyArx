# PyArx MfcXml 控件盘点与判定表（目标 1）

> 只读盘点：本轮未删除任何代码、未改源码、未构建、未跑 GUI 套件。
> 正式产物参照：`MfcXmlCore.dll 22:39:10` / `PyMfcXml.pyd 22:39:23`。
> 盘点基准提交：HEAD `3b5a25c`（工作树另有未提交改动，属另一取证线，本报告不评判）。

## 1. 盘点口径与统计范围

### 1.1 代码来源（全部给证据）

| 来源 | 实际位置（任务描述路径与实际有出入处已注明） |
| --- | --- |
| 注册表 | `MfcXmlCore\controls\RegisterPyMfcControls.cpp`（任务写作 `PyMfcXml\RegisterPyMfcControls.cpp`，实际在 MfcXmlCore\controls）。67 个控件类、69 个 `RegisterOne` 调用（含 1 个被 `isKnown` 守卫跳过的重复 XDrawer 注册）；`RegisterOne(reg, factory, {标签...})` 首标签为主名，其余为别名。 |
| 控件类 | `MfcXmlCore\controls\Xml*.h/.cpp`；另有 `MfcXmlCore\XmlDwgCanvasView.h`（header-only 模型）、`PyMfcXml\PyMfcDwgGL.*`（GL 后端，命名空间 `pymfcgl`，非类）。 |
| 标签分派 / 别名表 | `MfcXmlCore\XmlCore.cpp`：`KindOf()`（L1600 起，标签→内部短名）、`IsKnownControl()`（L1632）、`IsContainer()`（L1715）、`XmlControlRegistry`（L5228–5272）；`controls\XmlMfcTags.h`：`BareTag()` / `containerTags()` / `controlTags()` / `canonicalTag()`；`controls\XmlQmlNames.h`：属性名 QML 别名（非标签别名）。 |
| 设计器目录 | `UIDesign\control_catalog.xml`（80 条 `objectinfo`）；`UIDesign\ControlCatalog.cpp` 仅加载目录并注入缺省尺寸，不做实例化分派。 |
| Python 暴露面 | `PyMfcXml\PyMfcBind.cpp`：`Window` / `Pane` 宿主类 + `add*` 方法；`register_control` / `create_embedded` / `guiload` / `guiload_text` / `xmlLayout` / `prop_schema`；`PyMfcXml\XmlBindings.cpp`：`xml.Document` / `xml.Node` / `xml.tags()`；`MfcXmlRuntime\html.py`（HTML 标签→X 标签映射表）、`MfcXmlRuntime\runtime.py`（老 `_TAG_SPEC` 映射表）。 |

**Python 可达性结论**：pybind 未把任何 `Xml*` 控件类直接导出为 Python 类。Python 侧建控件只有两条路——① 宿主 `Window/Pane.add*` 老通道（覆盖 Button/Label/Edit/ListBox/CheckBox/Radio/Group/Combo/Progress/Slider/Tab/Tree/ListCtrl/Choice/DatePicker/Panel/Splitter/StaticLine/Hyperlink/RichText/Toggle/BitmapButton/StaticBitmap/Grid/ScrolledWindow/Calendar/Table/ToolBar/Menu/Popup/DwgCanvas/View3D 等子集，方法清单见 `PyMfcBind.cpp` L104–442）；② XML 字符串/文件经 `guiload`/`guiload_text`/`html()` 进注册表工厂建出，注册表内 67 类理论上都可由 XML 建出。`xml` 子模块只暴露 `Document/Node` 只读视图与 `tags()/defaultSize/parseColor`；`pp`（配电图布局）、`svg`（SVG 导入）子模块与控件注册表无关。`register_control(tag, factory)` 可追加 Python duck-typing 控件，不占用 C++ 注册表类位。

### 1.2 标签命中口径（"真支持" vs "只是出现过"）

- **真能被解析命中**：标签进入 `XmlControlRegistry`（`registerFactory` 注册），实例化路径在 `XmlCore.cpp` L5490–5501（`isKnown` → `create`）。注册表匹配为 `_wcsicmp` 大小写不敏感的**整串精确匹配**（L5239/5252/5261），不含前缀剥离；前缀剥离只发生在 `BareTag()`/`KindOf()` 的布局与短名分派层。
- **不算支持**：`docs_notes\*.md` / `docs\*.md` 中的标签清单、对照表、用法示例（文档枚举，未走解析）；catalog 中未注册的设计器条目；注释里的样例。
- **布局/外壳/图元标签**（不建 `IXmlControl`，本报告不参与控件删除判定）：`XWindow` / `XStackPanel` / `Grid` / `RowDefinition` / `ColumnDefinition` / `Pane` 容器 / `MenuBar` / `StatusBar` / `ShellTitle` / `ChipBar` / `Stage` / `TabStrip` 等外壳节点；`XLine` / `XRectangle` / `XEllipse` / `XArc` 走 `XmlLeafPaint.h` 图元绘制；`XRibbonTab/Group/Button/MenuItem/Backstage/BackstageItem` 是 `XRibbon` 模型子节点（`XmlRibbon.h` 结构体解析）；`XScene/XPolyline/XRect/XCircle/XText` 是 DWG 场景 XML 图元；`XQat/XQatButton/XShellTitle/XBorder/XControlTemplate/XChipBar/XStage/XTabStrip/XPanelHeader/XPaletteSet/XLayout/XCanvas` 在样例中出现但注册表无工厂，属设计器/外壳占位。

### 1.3 排除目录

`_attic`、`third_party`、`X64`、`build`、`Debug`、`Release`、`log`、`pylibs`、`train.venv`、`ACAD`（另含 `ACAD_min`、`新建文件夹`、`tmp_cap`、`GITEE令牌`、`CAD快速看图`、`viewer\_sdk` 第三方 WebView2 包）。
扫描文件 1302 个（`.xml/.py/.md`）。引用分桶：`ex`=examples，`ts`=tests，`UID`=UIDesign 样例/目录，`xml`=其他 XML（plugins/tools），`py`=其他 Python（tools/pyarx_vision/MfcXmlRuntime），`doc`=文档提及。每类最多列 5 条文件，其余记 `+N`。

### 1.4 判定枚举

`保留-核心` / `保留-低频` / `可合并` / `可删` / `仅别名`。

### 1.5 保护名单（铁律：不判可删、不改名）

`XmlShxText`、`XmlDwgCanvas`、`XmlDwgCanvasView`、`PyMfcDwgGL`、`XmlFlexPanel`、`XmlPanel`、`XmlMfcControlBase`。下文若认为其非必要，最高只标 `可合并` 并给收敛落点，不出删除方案。

---

## 2. 总表（按判定分组排序）

> 体量 = 对应 `.h` + `.cpp` 行数（`ReadAllLines` 计数，空 cpp 仅含 include 注释）。
> 命中数为**文件数**（非出现次数）；`doc` 列均为文档提及，不计入真实使用。

### 2.1 可删候选（G1 皮肤指示小件 / G2 04 批次薄壳）

| 类名 | 注册名（主标签 / 别名） | 代码体量 | ex | ts | UID | xml | py | doc | 继承 / 复用点 | 判定 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| XmlBusyIndicator | XBusyIndicator | h14+cpp7=21 | 0 | 0 | 1 | 0 | 0 | 3 | 继承 XmlMfcControlBase；仅 `XmlEvents.h` 有 `busyindicator` 事件名 | 可删（G1） |
| XmlPageIndicator | XPageIndicator | h27+cpp39=66 | 0 | 0 | 1 | 0 | 0 | 3 | 继承基类；复用 `setPageCount/setPageIndex` 宿主接口 | 可删（G1） |
| XmlToolTip | XToolTip | h26+cpp23=49 | 0 | 0 | 1 | 0 | 0 | 3 | 继承基类；宿主 `showToolTip/hideToolTip` 为老 API，未被本类以外路径引用 | 可删（G1） |
| XmlSwipeView | XSwipeView | h12+cpp2=14 | 0 | 0 | 1 | 0 | 0 | 3 | 继承 XmlPageView（空壳） | 可删（G1） |
| XmlTreeView | XTreeTable | h30+cpp70=100 | 0 | 0 | 1 | 0 | 0 | 3 | 继承基类；注意名实错位：`XTreeTable` 工厂是本类，`XTreeView` 工厂是 XmlTreeCtrl | 可删（G1） |
| XmlCodeEditor | XCodeEditor | h32+cpp58=90 | 0 | 0 | 1 | 0 | 0 | 3 | 继承基类；后端 `host->addCodeEditor` 落到 `CodeEditor.h`（2134 行老编辑器），删本类不动后端 | 可删（G1） |
| XmlFrame | XFrame | h26+cpp24=50 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类；`xmlframe` 命名空间（XFrameWindow）与本类无关，勿误删 | 可删（G2） |
| XmlDialog | XDialog | h28+cpp48=76 | 0 | 0 | 1 | 0 | 1 | 2 | 继承基类；`html.py` TAGS 表把 `dialog` 映到 XDialog | 可删（G2） |
| XmlDrawer | XDrawer | h28+cpp46=74 | 0 | 0 | 1 | 0 | 0 | 1 | 继承基类；**注册表二次注册被 `isKnown` 守卫跳过**，XDrawer 实际工厂是 XmlPopup（见 §5） | 可删（G2） |
| XmlRoundButton | XRoundButton | h27+cpp35=62 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |
| XmlDelayButton | XDelayButton | h28+cpp47=75 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |
| XmlToolButton | XToolButton | h28+cpp48=76 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |
| XmlToolSeparator | XToolSeparator | h26+cpp26=52 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |
| XmlTabButton | XTabButton | h27+cpp38=65 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |
| XmlScrollIndicator | XScrollIndicator | h28+cpp44=72 | 0 | 0 | 1 | 0 | 0 | 2 | 继承基类 | 可删（G2） |

### 2.2 可合并候选

| 类名 | 注册名 | 体量 | ex/ts/UID/xml/py/doc | 继承 / 复用 | 判定与合并方向 |
| --- | --- | --- | --- | --- | --- |
| XmlPopup | XPopup, XDrawer | 77 | 0/1/2/0/0/3 | 继承基类；XDrawer 别名同工厂 | 可合并：弹层族落点（见 §4.1） |
| XmlPage | XPage | 14 | 0/2/1/0/0/3 | 继承 XmlPageView | 可合并：页容器族（§4.2） |
| XmlStackView | XStackView | 14 | 0/4/1/0/0/3 | 继承 XmlPageView | 可合并：页容器族（§4.2） |
| XmlMenuItem | XMenuItem | 77 | 0/0/1/0/0/2 | 继承基类；XMenu 子项 | 可合并：菜单族（§4.4） |
| XmlMenuSeparator | XMenuSeparator | 52 | 0/0/1/0/0/2 | 继承基类；XMenu 子项 | 可合并：菜单族（§4.4） |
| XmlChoice | XChoice | 81 | 0/1/1/0/0/3 | 继承基类 | 可合并：输入族 → XmlComboBox（§4.3） |
| XmlCalendarCtrl | XMonthGrid | 56 | 0/2/1/0/0/3 | 继承基类 | 可合并：日期族 ↔ XmlDatePicker（§4.3） |
| XmlRangeSlider | XRangeSlider | 95 | 0/1/1/0/0/3 | 继承基类 | 可合并：输入族 → XmlSlider（§4.3） |
| XmlSpinBox | XSpinBox | 83 | 0/5/1/0/0/3 | 继承基类 | 可合并：数值输入族（§4.3） |
| XmlPathView | XPathView | 89 | 0/1/1/0/0/3 | 继承基类 | 可合并：列表/委托族（§4.5） |
| XmlGridView | XGridView | 114 | 0/2/1/0/0/3 | 继承基类 | 可合并：列表/委托族（§4.5） |
| XmlListView | XVirtualList | 103 | 0/2/1/0/0/3 | 继承基类 | 可合并：列表/委托族（§4.5） |
| XmlItemView | XItemView | 72 | 0/1/3/0/1/2 | 继承基类；委托行模板基位 | 可合并：列表/委托族收口于此（§4.5） |
| XmlToggleButton | XToggleButton | 65 | 0/3/1/0/1/3 | 继承基类 | 可合并：按钮族（§4.3） |
| XmlBitmapButton | XIconButton | 61 | 3/2/1/0/1/3 | 继承基类 | 可合并：按钮族（§4.3） |
| XmlSplitter | XSplitView | 21 | 0/2/2/0/2/3 | 继承基类；空壳 | 可合并：与 XmlPanel/Frame 容器层收敛（§4.6） |
| XmlGroupBox | XGroupBox | 51 | 2/1/1/0/1/3 | 继承基类 | 可合并：容器族（§4.6） |

### 2.3 仅别名（无独立实体或别名指向同工厂）

详见 §5。

### 2.4 保留-低频

| 类名 | 注册名 | 体量 | ex | ts | UID | xml | py | doc | 继承 / 复用 | 判定 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| XmlRadioButton | XRadioButton | 66 | 1 | 2 | 1 | 0 | 1 | 3 | 继承基类；`html.py`/`runtime.py` 有映射 | 保留-低频 |
| XmlListCtrl | XListCtrl | 73 | 0 | 2 | 1 | 1 | 0 | 3 | 继承基类；`runtime.py` `ListCtrl` 映射 | 保留-低频 |
| XmlDatePicker | XDatePicker | 52 | 0 | 2 | 1 | 0 | 0 | 3 | 继承基类 | 保留-低频 |
| XmlHyperlink | XHyperlink | 61 | 0 | 2 | 1 | 0 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlRichText | XTextArea | 100 | 0 | 2 | 1 | 0 | 2 | 3 | 继承基类 | 保留-低频 |
| XmlStaticBitmap | XImage | 49 | 1 | 1 | 2 | 1 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlGrid | XGridLayout | 73 | 0 | 1 | 1 | 0 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlScrolledWindow | XScrollView | 64 | 0 | 3 | 1 | 0 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlSwitch | XSwitch | 53 | 0 | 6 | 1 | 0 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlDial | XDial | 90 | 0 | 3 | 1 | 0 | 0 | 3 | 继承基类 | 保留-低频 |
| XmlTumbler | XTumbler | 76 | 0 | 1 | 1 | 0 | 0 | 3 | 继承基类 | 保留-低频 |
| XmlScrollBar | XScrollBar | 120 | 0 | 1 | 2 | 1 | 0 | 4 | 继承基类 | 保留-低频 |
| XmlToolBar | XToolBar | 86 | 0 | 0 | 1 | 0 | 2 | 3 | 继承基类；`runtime.py` 老工具条映射 | 保留-低频 |
| XmlMenu | XMenu | 63 | 0 | 1 | 1 | 0 | 0 | 3 | 继承基类；菜单族宿主 | 保留-低频 |
| XmlMenuBar | XMenuBar | 73 | 0 | 2 | 1 | 0 | 0 | 3 | 继承基类 | 保留-低频 |
| XmlQCanvas | XQCanvas | 69 | 0 | 8 | 1 | 1 | 0 | 3 | 继承基类；`setCanvasCommands` 通路 | 保留-低频 |
| XmlPage | （见 §2.2 可合并；保留实体不删） | 14 | 0 | 2 | 1 | 0 | 0 | 3 | XmlPageView 子类 | 保留-低频 |
| XmlFlickable | XFlickable | 64 | 0 | 1 | 1 | 0 | 0 | 3 | 继承基类 | 保留-低频 |
| XmlView3D | XView3D | 61 | 0 | 0 | 0 | 0 | 2 | 0 | 继承基类；后端 `addView3D/setView3DModel`；`tools\cadmcp\view3d_xml_demo.py`、`strong3d_native.py` 调用 | 保留-低频 |
| XmlRibbon | XRibbon | h23+cpp8=31（h 为超长单行，36180 字节） | 4 | 0 | 1 | 0 | 2 | 2 | 继承基类；模型结构体在 `XmlRibbon.h`；UIDesign `DesignCanvas.h/DesignerPaneSurface.cpp/DesignerPaneView.h` 引用 | 保留-低频 |
| XmlStaticLine | XDivider | 21 | 3 | 1 | 3 | 0 | 5 | 3 | 继承基类；空壳分隔线 | 保留-低频 |
| XmlTabControl | XTabBar | 69 | 0 | 1 | 1 | 0 | 1 | 3 | 继承基类 | 保留-低频 |
| XmlTreeCtrl | XTreeView | 58 | 0 | 1 | 2 | 0 | 2 | 3 | 继承基类；`html.py`/`runtime.py` 树映射 | 保留-低频 |

### 2.5 保留-核心

| 类名 | 注册名 | 体量 | ex | ts | UID | xml | py | doc | 继承 / 复用 | 判定 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| XmlButton | XButton | 52 | 10 | 22 | 10 | 2 | 6 | 8 | 继承基类；按钮族基位 | 保留-核心 |
| XmlTextBlock | XLabel | 51 | 13 | 35 | 15 | 2 | 11 | 6 | 继承基类；`html.py` 多标签归一到 XLabel | 保留-核心 |
| XmlTextBox | XTextField | 182 | 5 | 16 | 3 | 0 | 2 | 6 | 继承基类 | 保留-核心 |
| XmlListBox | XListBox | 130 | 5 | 2 | 2 | 0 | 1 | 3 | 继承基类 | 保留-核心 |
| XmlCheckBox | XCheckBox | 66 | 3 | 8 | 2 | 0 | 1 | 3 | 继承基类 | 保留-核心 |
| XmlComboBox | XComboBox | 178 | 4 | 5 | 2 | 0 | 2 | 3 | 继承基类；输入族收拢位 | 保留-核心 |
| XmlProgressBar | XProgressBar | 81 | 2 | 8 | 1 | 0 | 1 | 3 | 继承基类 | 保留-核心 |
| XmlSlider | XSlider | 81 | 3 | 4 | 2 | 0 | 1 | 3 | 继承基类；滑杆族收拢位 | 保留-核心 |
| XmlTableView | XTableView | 113 | 3 | 5 | 1 | 0 | 1 | 8 | 继承基类；表格族主位 | 保留-核心 |
| XmlPanel | XPane | 21 | 0 | 1 | 9 | 0 | 0 | 3 | **保护名单**；继承基类；容器基位 | 保留-核心 |
| XmlFlexPanel | XFlexPanel / FlexPanel / FlexBox / Row / Column | 1086 | 9 | 42 | 3 | 5 | 80 | 12 | **保护名单**；`XmlCore.cpp` L5531 `dynamic_cast<XmlFlexPanel*>` 自持有子项；`PyMfcHost.cpp`/`PyMfcLoad.cpp`/`XmlCore.h`/基类头引用 | 保留-核心 |
| XmlDwgCanvas | XDwgCanvas | 88 | 6 | 7 | 1 | 0 | 7 | 10 | **保护名单**；模型 `XmlDwgCanvasView.h`（295 行）；GL 后端 `PyMfcDwgGL.cpp`（518 行，命名空间 `pymfcgl`，无 pybind 导出） | 保留-核心 |
| XmlShxText | XShxText / ShxText | 244 | 0 | 0 | 0 | 2 | 1 | 2 | **保护名单**；后端 `ShxFontCore.cpp`（778 行）+ `ShxFontCore.h`（126 行）；本轮刚修导出宏并完成 GUI 验证 | 保留-核心 |
| XmlMfcControlBase | （不注册 XML 标签） | 606 | — | — | — | — | — | — | **保护名单**；全部控件基类，实现 `IXmlControl` 公共骨架 | 保留-核心 |

---

## 3. 可删候选（本轮不删，等用户拍板）

> 原则：一次性只删一组；每组独立回滚；以下文件清单均已排除 §1.3 目录。
> 真实使用口径：examples/tests/plugins XML 命中为 0；Python 侧 G1 为 0，G2 仅 `html.py` 一条标签映射（非直接建控件）；UIDesign 样例各 1 份 + 文档枚举。

### 3.1 G1 — 皮肤指示小件（6 个，建议第一批）

**成员**：XmlBusyIndicator、XmlPageIndicator、XmlToolTip、XmlSwipeView、XmlTreeView(XTreeTable)、XmlCodeEditor。

**删后影响面（引用文件清单）**
- UIDesign 样例（每类仅 1 份）：BusyIndicator/PageIndicator/CodeEditor → `01_basic_controls.xml`；SwipeView/TreeTable → `02_data_controls.xml`；ToolTip → `03_overlay_canvas.xml`；`UIDesign\control_catalog.xml` 对应 6 条 `objectinfo`。
- 文档（仅枚举，删后需同步清理）：`docs_notes\mfcxml-控件接口清单.md`、`mfcxml-控件用法示例.md`、`mfcxml-控件中文名对照表.md`。
- C++ 内部：`controls\XmlEvents.h` 对应 `busyindicator` / `pageindicator` / `tooltip` / `swipeview` / `treetable` / `codeeditor` 事件名条目；`controls\XmlExtraControls.h` 对应 include；`RegisterPyMfcControls.cpp` 对应 `RegisterOne` 行。
- 无 examples / tests / plugins XML / Python 调用方命中。

**回滚方式**
- 版本基线：HEAD `3b5a25c`。回滚 = `git checkout 3b5a25c -- <下列文件>`，恢复被删 `.h/.cpp` 与注册表/目录条目。
- 需删/改文件（若执行）：6 对 `controls\Xml*.h/.cpp`；`RegisterPyMfcControls.cpp` 删 6 个 `RegisterOne`；`XmlExtraControls.h` 删 6 行 include；`XmlEvents.h` 删 6 个事件名；`control_catalog.xml` 删 6 条；样例 XML 删对应节点。
- **为什么第一批**：体量最小（21–100 行/个）；Python 宿主类无对应直接 `add*` 建出方法（底层 skinbox/tooltip 接口仅被本类内部调用）；无测试与示例覆盖、互不继承；删任一个不影响其余 5 个的编译与解析。

### 3.2 G2 — 04 批次薄壳（9 个，建议第二批）

**成员**：XmlFrame、XmlDialog、XmlDrawer、XmlRoundButton、XmlDelayButton、XmlToolButton、XmlToolSeparator、XmlTabButton、XmlScrollIndicator。

**删后影响面（引用文件清单）**
- UIDesign 样例：`UIDesign\samples\04_qml_controls.xml`（Frame/Dialog/Drawer/RoundButton/DelayButton/ToolButton/ToolSeparator/TabButton/ScrollIndicator 同文件出现）；`control_catalog.xml` 对应 9 条。
- Python 侧：`MfcXmlRuntime\html.py` TAGS 表 `'dialog': 'XDialog'` 一条映射；`tools\migrate_tags_qml_names2.py` 等迁移脚本仅历史改造用，非运行时。
- 文档：`docs_notes\mfcxml-控件接口清单.md`、`mfcxml-控件中文名对照表.md`（Dialog 另见 `mfcxml-控件用法示例.md`）。
- C++ 内部：`XmlEvents.h` 对应 `frame/dialog/drawer/roundbutton/delaybutton/toolbutton/toolseparator/tabbutton/scrollindicator`；`XmlExtraControls.h` include；注册表 9 行。
- 注意：XmlDrawer 注册项当前**未生效**（XDrawer 已被 XmlPopup 首注占用，见 §5），删它不改变现有解析行为。

**回滚方式**
- 同 G1：`git checkout 3b5a25c -- <文件>` 恢复。
- 需删/改文件：9 对 `.h/.cpp`；注册表 9 行；`XmlExtraControls.h` 9 行；`XmlEvents.h` 9 个事件名；`control_catalog.xml` 9 条；`html.py` 删 `dialog` 映射；样例节点清理。
- **为什么第二批**：与 G1 无继承依赖，但 `XDialog` 被 `html.py` 映射引用、`XmlDrawer` 与 XmlPopup 别名纠缠，需先确认 G1 落地后界面无回归，再动 G2。

### 3.3 本轮处置

**以上两组本轮不删，仅出候选清单；任何删除动作等用户拍板后单独成批执行。**

---

## 4. 可合并候选（方向 / 落点 / 代价）

### 4.1 弹层族：XmlPopup / XmlDialog / XmlDrawer
- 现状：三者都是 `addPanel`/`addSkinBox` + 属性表薄壳；XmlPopup 同时吃 `XPopup` 与 `XDrawer` 两个标签；XmlDrawer 类注册被守卫跳过，实际不可达。
- 合并方向：`XmlDialog` / `XmlDrawer` 收敛为 `XmlPopup`（或 XmlPanel 容器层）的标签别名；弹层差异（Title/Modal/Edge/Position）用属性表字段区分，不再各立 C++ 类。
- 落点：`controls\XmlPopup.{h,cpp}`；注册表只保留 `XPopup`，`XDialog`/`XDrawer` 作别名注册到同工厂。
- 代价：需核对 `addPanel` 与 `addSkinBox` 皮肤号语义；`html.py` 的 `dialog` 映射改指别名；catalog 三条目合并。

### 4.2 页容器族：XmlPage / XmlStackView → XmlPageView
- 现状：两个子类 `.cpp` 仅一行 include，实体逻辑全在 `XmlPageView`（58 行）；XmlPage 另被 `XPage` 注册。XmlSwipeView 同属空壳子类，但已列入 §3.1 G1 可删候选，不在本合并方向内。
- 合并方向：去掉两个空子类，`XPage`/`XStackView` 别名直挂 XmlPageView；翻页方向/动画差异留属性。
- 落点：`controls\XmlPageView.{h,cpp}`；删除 `XmlPage`/`XmlStackView` 两对空壳文件。
- 代价：`XmlCore.cpp` 若有按类名 `dynamic_cast` 的分支需一并改口径（当前仅 XmlFlexPanel 有此 cast）；`demo_pager.py`/`demo_page_anim.py` 等测试用标签名保持不变。

### 4.3 按钮 / 输入族收敛
- 按钮族：XmlRoundButton / XmlDelayButton / XmlToggleButton / XmlBitmapButton(IconButton) / XmlToolButton / XmlTabButton / XmlToolSeparator → XmlButton，用 `Shape`（Rect/RoundRect/Ellipse/Capsule）、`Checkable`、`Delay`、`Icon` 属性区分；落点 `controls\XmlButton.{h,cpp}`。代价：宿主 `addToggle/addBitmapButton` 等老 `add*` 通道需保留适配层。
- 输入族：XmlChoice → XmlComboBox；XmlRangeSlider → XmlSlider；XmlSpinBox 并入 XmlSlider/XmlTextBox 数值分支；XmlCalendarCtrl ↔ XmlDatePicker 二选一保留。代价：属性表字段与 `runtime.py` 老 `_TAG_SPEC` 映射要同步，避免老插件 XML 失配。

### 4.4 菜单族：XmlMenuItem / XmlMenuSeparator → XmlMenu 模型
- 现状：MenuItem/MenuSeparator 是 XMenu 的子项薄壳；模型结构已在 `XmlRibbon.h` 的 `XRibbonButton::Item` 附近。
- 合并方向：子项不再注册为独立控件类，作为 XmlMenu 解析期的行模型；落点 `controls\XmlMenu.cpp` + 菜单模型结构体。
- 代价：菜单/工具条子项的命中、事件路由要从控件树改为模型遍历；改动面中等，建议在 §3 删除批次之后再做。

### 4.5 列表 / 委托族：XmlItemView 为收口位
- 现状：XmlItemView（委托行模板）、XmlListView(XVirtualList)、XmlGridView、XmlPathView、XmlTableView、XmlListCtrl、XmlTreeCtrl、XmlTreeTable 各自一套属性表。
- 合并方向：行模板/委托统一走 XmlItemView 模型，列表类只保留数据源与渲染差异；代价最大，列为远期合并，本轮不动代码。

### 4.6 容器族：XmlSplitter / XmlGroupBox → XmlPanel
- 现状：XmlSplitter 21 行空壳；XmlGroupBox 51 行；XmlPanel 为保护名单容器基位。
- 合并方向：分隔条/分组框作为 XmlPanel 的 `Kind` 与皮肤号；代价低，可随 4.1 一并评估。

### 4.7 保护名单内建议收敛项（仅收敛，不出删除方案）

- `XmlPanel`：当前 21 行空壳，可作为 §4.3/4.6 容器族唯一落点；不删类、不改标签。
- `XmlMfcControlBase`：606 行公共骨架，建议后续把各子类重复的属性表样板下沉到基类模板方法；属结构优化，非删除。
- `XmlFlexPanel` / `XmlDwgCanvas` / `XmlShxText`：本轮无收敛建议，维持现状。

---

## 5. 仅别名（标签别名，无独立实体或同工厂）

| 标签 | 指向工厂 / 实现 | 口径说明 |
| --- | --- | --- |
| `FlexPanel` / `FlexBox` / `Row` / `Column` | XmlFlexPanel 同工厂 | `RegisterPyMfcControls.cpp` L93 注册；`Row`/`Column` 是短名别名，解析时整串 `_wcsicmp` 精确匹配，不会命中 `RowDefinition`/`ColumnDefinition`。 |
| `XDrawer` | XmlPopup 同工厂（首注生效） | L156–159 先注 XmlPopup→XPopup/XDrawer；L192–193 XmlDrawer 再注 XDrawer 时被 `isKnown` 守卫跳过（L44）。**XmlDrawer 类当前不可达**。 |
| `ShxText` | XmlShxText 同工厂 | L220；与 `XShxText` 同工厂。 |
| 布局层短名别名（非控件注册） | `KindOf()` L1615–1628 | `TextBlock→label`、`TextBox→edit`、`RadioButton→radio`、`GroupBox→group`、`TreeView→treeview` 等；`BareTag()` 统一去 `{ns}` 与 `X` 前缀，`canonicalTag()` 还原为 `X+本意名`。 |
| 名实错位两条（核对时勿混） | — | `XTreeView` 工厂 = XmlTreeCtrl；`XTreeTable` 工厂 = XmlTreeView。 |
| 属性名别名（非标签） | `XmlQmlNames.h` | QML 写法 → 内部属性名（`text→Text`、`color→Foreground` 等），与控件标签无关。 |

---

## 6. 结论与建议批次

1. **第一批（风险最低）= G1 皮肤指示小件 6 个**：无宿主 `add*` 绑定、无 examples/tests/plugins 命中、互不继承、单个体量 ≤100 行；先删这批对现有界面解析与 Python 调用面零回退风险。
2. **第二批 = G2 04 批次薄壳 9 个**：依赖 G1 落地后回归验证；注意先处理 `html.py` 的 `dialog→XDialog` 映射与 XmlDrawer/XmlPopup 别名纠缠。
3. **合并先于删除扩展**：弹层族（Popup/Dialog/Drawer）与页容器族（Page/StackView/SwipeView）是最划算的两处合并，文件少、逻辑薄，建议在任何新删除批次前先做收口。
4. **列表/委托族合并排最后**：ItemView/ListView/GridView/PathView/TableView 一族改动面大，且 `XItemView` 在 UIDesign 委托样例（11/13/14 号 XML）有真实使用，本轮只登记方向。
5. **catalog 与代码必须同步**：`control_catalog.xml` 条目、`XmlEvents.h` 事件名、`XmlExtraControls.h` include、注册表 `RegisterOne` 行是四处联动点；删/合并任一控件都要四处一起改，否则设计器目录与运行时注册表漂移。
6. **Python 暴露面现状**：能被 Python 直接 `add*` 建出的是老基础控件子集；新批次控件全部走 XML（`guiload_text`/`html()`）解析建出。`register_control` 可挂 Python duck-typing 控件，不影响 C++ 注册表判定。
7. **保护名单维持**：XmlShxText（本轮刚修 + GUI 验证）、XmlDwgCanvas/XmlDwgCanvasView/PyMfcDwgGL（刚机械改名）、XmlFlexPanel（flex 主线）、XmlPanel/XmlMfcControlBase（基座）均不进入删除候选；其中 XmlPanel、XmlMfcControlBase 可在后续做结构下沉收敛。
8. **未决核对项**：`XQat/XQatButton/XShellTitle/XBorder` 等设计器占位标签在注册表无工厂，拖入画布后不会建控件；建议后续在 catalog 标注"外壳占位"或补注册，避免设计器与运行时口径不一致。

## 目标2：渲染机制与性能取证

### 8.1 静态取证（代码位置均为实际行号）

| 现象 | 证据位置 | 机制 |
| --- | --- | --- |
| 外观设值无幂等短路 | `PyMfcXml\PyMfcHost.cpp:7215` `setControlAppearance` | 入口即 `m_ap[id] = ap`，**不比较新旧是否相同**；随后无条件 `m_backdropSyncDirty = true`、删旧 `m_apBrush` 再 `CreateSolidBrush`、`releaseContainerPatternBrush(id)` 丢缓存位图、`InvalidateRect(h, nullptr, TRUE)` 整控件失效。同一份外观被重复下发 N 次即产生 N 次刷重建 + N 次全控件重绘。 |
| 属性逐项触发外观下发 | `MfcXmlCore\controls\XmlMfcControlBase.cpp:103` | 属性写入路径回调 `m_host->setControlAppearance(m_id, m_ap)`，故一个控件写 K 个外观属性 = K 次上式全套动作（与 `XmlShxText::setOwnProperty` 每属性各 `rebuild()` 同源）。 |
| 整窗全量重排 | `PyMfcXml\PyMfcHost.cpp:4098` `reflowXml` | 每次调用都 `applyExpressions()`（求值全树）→ `m_xml->layout(0,0,w,h)`（全树布局）→ 递归 `walk` 全树，逐节点 `moveControl(previewX/Y/W/H)`；**未比较 `::GetClientRect` 结果是否变化**，也无「布局属性未变则跳过」判据。 |
| 单控件搬运无变化短路 | `PyMfcHost.cpp` `moveControl`（`reflowXml` L4131 调用点） | 逐节点无条件 `SetWindowPos`，矩形相同也照搬。 |

### 8.2 可量化目标（下一轮实现后回填实测）

- `reflowXml` 空转（客户区未变 + 无表达式）：`moved` 次数应由 O(N) 降到 0；宿主 CPU 单次耗时作为对照量。
- 同一外观重复下发 10 次：`CreateSolidBrush` 调用数应由 10 次降到 1 次（可挂计数器或在 DBG 日志里数 `[ap]` 行）。
- 既有关联指标可直接复用：`examples\cad2d\xml_view.py --batch` 的 `canvas 1032x761 / scene 570.4 KB · k=0.00101 / line=1292 pline=3478 circle=450 arc=56 text=3 block=1606 skip=16406`。

---

## 7. 执行记录：第 1 批已落地（目标 1）

### 7.1 实际删除（7 个，均先备份）

| 类名 | 注册标签 | 判定依据 | 处置 |
| --- | --- | --- | --- |
| XmlBusyIndicator | XBusyIndicator | 只在 `UIDesign\samples\01_basic_controls.xml` + catalog 出现 | 类文件、注册、事件名、catalog、样例节点全删 |
| XmlPageIndicator | XPageIndicator | 同上 | 同上 |
| XmlToolTip | XToolTip | 只在 `03_overlay_canvas.xml` + catalog | 同上（`html.py` 里 `tooltip` 是**属性名**映射，与标签无关，无需改） |
| XmlSwipeView | XSwipeView | 只在 `02_data_controls.xml` + catalog | 同上 |
| XmlTreeView | XTreeTable（注意不是 XTreeView，后者是 XmlTreeCtrl） | 只在 `02_data_controls.xml` + catalog | 同上 |
| XmlCodeEditor | XCodeEditor | 只在 `01_basic_controls.xml` + catalog | 同上（`controls\CodeEditor.h` 是另一文件，保留） |
| XmlDrawer | （不可达） | XDrawer 首注被 XmlPopup 占用，`isKnown` 守卫跳过，类从未实例化 | 删类与死注册；**XDrawer 标签保留**（仍映射 XmlPopup），`04_qml_controls.xml` 不动 |

### 7.2 联动改动点

`RegisterPyMfcControls.cpp`（62 个 `RegisterOne` 剩余，-7）、`XmlExtraControls.h`（include 47→42 行）、`XmlEvents.h`（406→403 行，删 codeeditor / treetable / swipeview / pageindicator / tooltip / busyindicator / drawer 事件名）、`MfcXmlCore.vcxproj`（214→201 行）、`UIDesign\control_catalog.xml`（6 条 objectinfo）、三个样例 XML 的对应节点。

### 7.3 验证

- 构建：`MSBuild PyMfcXml.vcxproj /p:Configuration=Release /p:Platform=x64` **EXIT=0**，日志 `log\build_audit_233913.txt`。
- 注册表：`xml.tags()` = 66 条，被删标签 0 残留；保护名单 `XDwgCanvas / XShxText / XFlexPanel / XButton / XPopup / XDrawer` 均在位。
- XML 完整性：`UIDesign\samples\*.xml` + `control_catalog.xml` 共 15 个文件全部 well-formed。
- 红线：`examples\cad2d\xml_view.py --batch` 计数与基线逐项一致（`line=1292 pline=3478 circle=450 arc=56 text=3 block=1606 skip=16406`，`canvas 1032x761`，`scene 570.4 KB · k=0.00101`）。

### 7.4 回滚

`G:\PyArx\log\del_batch1_backup\`（22 个文件，含 7 对 `.h/.cpp` 与被改的注册表/事件表/工程/目录/样例）。

### 7.5 下一批（待定，未执行）

§3.2 的 G2 九件薄壳（Frame/Dialog/Drawer/RoundButton/DelayButton/ToolButton/ToolSeparator/TabButton/ScrollIndicator）。其中 XmlDrawer 已在第 1 批删除；`XDialog` 仍被 `MfcXmlRuntime\html.py` 的 `dialog` 标签映射引用，删前需把该映射改指 `XPopup`。
