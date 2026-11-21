# MfcXml 控件封装：评审结论与整改意见

> 评审对象：`G:\PyArx\MfcXmlCore`（控件库）、`G:\PyArx\PyMfcXml`（宿主 + pybind）、
> `G:\PyArx\MfcXmlRuntime`（复合控件运行时）、`G:\PyArx\Viewer`、`G:\PyArx\UIDesign`。
> 评审时间：2026-11-13。全部结论按源码逐条核对，不引用文档口径。

---

## 0 一句话结论

封装方向（XAML 声明 + C++ 控件 + 回调进 Python）是对的，问题在**每加一个控件要改 5 处**、
**属性和事件各存在 3~4 份平行定义且已漂移**、**复合控件的行为长在宿主里而不在控件类里**。
不是"控件不够多"，是"控件的归属没定"。

---

## 1 已落地、本次不再重复的项

| 项 | 证据 | 状态 |
|---|---|---|
| 复合层 XAML 方言统一（`x:Name` / `Name` / `id` 互通） | `MfcXmlRuntime\runtime.py:32` `_TAG_SPEC` | 已做 |
| 复合层事件扩到 9 种 + 6 类控件级事件派发 | `runtime.py:75` `EVENT_SPEC`、`:107` `CTL_EVENTS`；宿主 `PyMfcEventSubProc` | 已做 |
| 表格事件按控件 id 分流（多表不串台） | `onGridActionFor` / `fireGridAction(ctlId,…)` | 已做 |
| 读写对称 `getXmlValue` | `PyMfcBind.cpp` Window/Panel 各一 | 已做 |
| Table 水平滚动（`m_colScroll` / `Shift+滚轮`） | `PyMfcXml\Table.cpp` 15 处 | 已做 |
| 历史实现归档 | `docs\_history\*` | 已做 |
| `MFCXml 此文件夹项目暂停修改` | 实测 **0 个子项**，空目录 | 建议直接删 |

---

## 2 仍未解决的七个问题

### P0-1 XAML 正式加载路径的事件仍然只接 3 种（与复合层不对称）

`PyMfcLoad.cpp:783-805`、`:1118-1133` 的注释与实现都只有 `Click / TextChanged / SelectionChanged`，
`guiLoad` 不认 `CheckedChanged`、`ValueChanged`、`Scroll`、`MouseDown`、`KeyDown`…

- 后果：XAML 里写 `CheckedChanged="onX"` 不报错、也不触发（`XmlMfcControlBase.cpp:218` 先把它
  存进 `m_eventNames`，谁都不派发）。复合层（`runtime.py`）已经能派发 9+6 种，**两条路径行为不一致**。
- 判断：宿主侧派发能力已就绪（`onControlEvent` 已存在），缺的是 `guiLoad` 的解析绑定表。
  修法：把 `PyMfcLoad.cpp` 的 3 段 if 换成一张 `kind → 绑定函数` 表，那张表由控件事件契约导出。

### P0-2 事件契约按"类型名字符串"分支，30+ 控件的 `events()` 返回空

`XmlEvents.h:109` 的 `xmlEventsForControl` 只覆盖 18 个名字（button/combo/tab/tree/grid/label…），
而控件构造用的是另一套短名（`tableview` `treeview` `switch` `spinbox` `dial` `tumbler`
`scrollbar` `virtuallist` `qcanvas` `gridview` `pathview` `menubar` `toolbar` `popup`…，见各 `Xml*.h` 构造函数）。
该文件 `LastWriteTime = 2026-11-07`，本轮事件改动没有覆盖它。

- 后果：设计器属性面板对 30+ 控件"一个事件都没有"，XAML 校验也无法据此提示。
- 判断：事件"该有哪些"属于控件自己，不该由名字 switch 决定。

### P0-3 属性三处手写，且已漂移

每个属性要写元数据 + setter + getter，全库 **179 处 `xmlprop::StrEq`**，每次读写都 `Lower(WideToUtf8(name))` 再线性比对：

| 漂移实例 | 证据 |
|---|---|
| `Name` 被声明成控件属性（它是文档节点身份） | `XmlScrollBar.cpp:12`、`XmlTableView.cpp:13` |
| `Content`/`Text` 两个属性指向同一存储，面板出现重复项 | `XmlTextBox.cpp:13-14` |
| `Orientation` 声明 String 而非 Enum（ComboBox 却用 Enum） | `XmlScrollBar.cpp:17` |
| 未知属性静默丢弃，无诊断 | `XmlMfcControlBase.cpp:234` 直接 `return setOwnProperty(...)` |
| "无此属性"与"值为空串"不可区分 | `getOwnProperty` 统一 `return {}` |

- 判断：这是**属性面板、XAML 校验、Python 读写三处不一致的总源头**，优先级高于任何单控件修复。

### P0-4 复合控件空心化，真逻辑住在宿主里

51 个控件类中 **39 个 `createNative` 只有一行 `host->addXxx(...)`**。原生 Win32 控件这样转发合理，
但 `TableView/TreeView/GridView/ListView/PathView/Popup/StackView/Splitter/SpinBox/Dial/Tumbler`
这些自绘复合控件也只剩转发——状态机、命中、滚动、编辑态都在 `PyMfcHost.cpp`（6130 行）。

- `Table.cpp` 1531 行躺在 **`PyMfcXml`（Python 绑定工程）** 里，不在 `MfcXmlCore`：Viewer / UIDesign 拿不到它。
- 硬耦合：`XmlTableView.cpp:6` 写 `addSkinBox(id, 9, …)`，`9` 的含义是 `PyMfcHost.h:59 kSkinBoxTable = 9`。
  22 种 kind 全靠魔法数字与宿主私有常量手工对齐，core 库无法独立验证。
  （另：`addTable` 只存在于 Python 侧 `PyMfcWindow::addTable`，注册表里没有对应 tag，同一控件两条身份。）

### P1-1 宿主是 159 个虚函数的胖接口，横向复制 4 套

`IXmlMfcHost`：49 个纯虚 + 110 个默认实现 = 159 个虚函数。实现者：

| 实现 | 规模 |
|---|---|
| `PyMfcHost` | 头 945 行 / 实现 6130 行 / 窗口过程 2283 行，还兼职 tab hook、splitter、dragbar、owner-draw 子类化 |
| `ViewerMfcHost` | `XmlViewerMfcHost.h`，63 个 override |
| `ViewerNativeHost` | `XmlViewerNativeHost.h`，61 个 override |
| `CDesignCanvas` | `UIDesign\DesignCanvas.h`，GDI/D2D 自己再画一遍预览 |

- 后果：同一份 XAML 有 4 套像素路径，设计器所见不等于运行所得；新增控件要牵动这三台宿主。

### P1-2 身份是魔法 ID，`create()` 是空承诺

- `IXmlControl::create(parentHandle)` 什么都不建却 `return true`（`XmlMfcControlBase.cpp:123`），
  真正创建走另一条 `attach(host,id,…)`，`parentHandle` 直接丢弃；
- 身份是 `1000..65000` 的 int，基类与宿主各写一遍区间判断（`XmlMfcControlBase.cpp:80,110,132`）；
- `destroy()` 直接 `DestroyWindow`，与宿主自身销毁路径重叠；
- 注册表是 `vector` 线性查找（`XmlCore.h:137`），别名泛滥且已三次发生"别名抢占"
  （`RegisterPyMfcControls.cpp:119-126` 注释即事故记录）。

### P2 缺单元级保护

`tests\mfc_xml` 全是端到端截图/像素比对，控件层（属性表 round-trip、事件契约覆盖、tag 唯一性）零测试。

---

## 3 整改方案（按落地顺序，前两步不动 UI）

### S1 一张表定属性（风险最低、收益最大）

```cpp
struct PropDesc {
    const wchar_t* name; const wchar_t* aliases; PropType type;
    std::function<void(Ctx&, const Value&)> set;
    std::function<Value(const Ctx&)> get;
    const wchar_t* enumValues;
};
```

- 类型系统补齐 `Double / Point / Rect / Thickness / Image / Columns / Rows / Flags`；
- 数字解析收成一处（现有 `_wtoi` / `xmlExtraNum` / `propIntOf` 三套并存）；
- 未知属性、未实现属性返回诊断码，加载期汇总成 warning 给设计器；
- 顺带清掉 `Name`、`Content/Text` 重复项，`Orientation` 改 Enum。

**验收**：179 处 `StrEq` 归零；51 个控件属性 round-trip 单测全绿；属性面板不再出现重复/伪属性。

### S2 事件契约下沉 + 事件表只留一份

- 删 `xmlEventsForControl`，改每控件 `virtual std::vector<XmlEventKind> events()`；
- kind ↔ 属性名 ↔ payload 只在 `XmlEvents.h` 一份，并**导出 schema(json)**；
- `PyMfcLoad.cpp` 的三段 if 改为读 schema 生成绑定；`runtime.py` 的 `EVENT_SPEC/EVENT_ALIASES` 由 schema 生成。

**验收**：XAML 里写任一控件声明过的事件都真派发；`CheckedChanged` 不再是静默黑洞；两条路径（guiLoad / Composite）事件集合完全一致。

### S3 宿主瘦身成"能力集"

```cpp
struct IHostServices {  // 只留真正通用的能力
    Surface* createNative(ControlKind, const CreateInfo&);
    void invalidate(Surface*); void timer(...); IFont* font(); ITheme* theme();
};
```

- 表格行/列、画布场景、页容器这些接口从 `IXmlMfcHost` 摘出，回到控件自己的头（`ITableView` 等）；
- `kSkinBox*` 枚举从 `PyMfcHost.h` 提到 core 的 `XmlControlKind.h`，控件用枚举不用 `9`；
- `create/attach` 合成一条，身份改 `Surface*`。

**验收**：新增一个控件的 diff 只需 2 个文件（控件 + 注册行）；`PyMfcHost.cpp` 从 6130 行降到 ≤ 2000。

### S4 复合控件下沉 + 单一渲染

- `Table.cpp` 及表格/树/页容器/浮层实现从 `PyMfcXml` 搬进 `MfcXmlCore\controls`，状态由控件类持有；
- `plugins\` 只保留纯 XAML 组合型复合控件，其 tag/属性/事件表由 registry 导出；
- skin box / 圆角阴影 / 表头外观抽成 `XmlSkinPainter`（core），运行时、Viewer、`CDesignCanvas` 共用同一份；
  设计器只保留选框/夹点/网格这类编辑装饰。

**验收**：`<XTableView Columns="…"/>` 在三处像素一致；`pp_app` 里不再出现 `E_F0+i`、`B_APPLYROW` 这类手工摆位 id。

### S5 注册表与测试

- `std::unordered_map` + 单一 tag，别名表独立，启动时冲突直接抛错；
- 无窗口单测：属性 round-trip、事件契约全控件覆盖、tag/别名唯一、`ControlKind` 与控件声明一致；
- 把 `_perf_ctl.py` / `_perf_table.py` 的三条基线（整窗 ≤20ms、表格 ≤3ms、fill 39 行 ≤5ms）留成 CI 门槛。

---

## 4 建议的第一步（给施工线的指令）

> 先做 S1 + S2，都是纯内部改造、UI 零变化，一次编译一次回归即可。
> 具体第一刀：把 `XmlTypes.h`（PropType + PropDesc + Value）和 `XmlEvents.h` 的 schema 导出落地，
> 挑 `XmlTextBox` / `XmlComboBox` / `XmlTableView` 三个当样板改完，量一下 `StrEq` 与属性表规模，
> 再决定是否一次性铺到 51 个控件。

不建议先做的事：直接补完 51 个控件的事件、或先把 `IXmlMfcHost` 拆掉——前者会在 S1 落地后重写一遍，
后者要同时改三台宿主，风险集中且目前没有单测兜底。

---

## 5 执行记录（S1 + S2 已落地，2026-11-13）

### 改了哪些文件

| 文件 | 改动 |
|---|---|
| `MfcXmlCore\XmlCore.h` | `XmlPropType` 追加 `Double / Point / Rect / Thickness / Image / Columns / Rows / Flags`，属性表用它标注语义类型 |
| `MfcXmlCore\controls\XmlTypes.h`（新增） | `XmlPropEntry`（Name / Aliases / Type / EnumValues / Set / Get）+ `xmlprop2::` 的 `MatchesName`、`Trim / ToInt / ToDouble / ToBool / FromBool / FromInt / SplitBars`、未知属性诊断槽 |
| `MfcXmlCore\controls\XmlEvents.h` | `xmlEventsForControl` 的 if 链换成一张覆盖全部控件短名的表（`xmlDefaultEvents`）；新增 `XmlEventInfo` 表（property / key / payload / hostDispatch）与 `xmlEventSchemaJson()` |
| `MfcXmlCore\controls\XmlMfcControlBase.h/.cpp` | `events()` 改为 virtual；新增 `ownPropTable()` 钩子；`properties()/setProperty()/getProperty()` 统一"表优先、旧路径兜底"，别名匹配，`Set==nullptr` 即只读；未命中的非布局属性进诊断表 |
| `XmlTextBox.h/.cpp` | 属性表：Text（别名 Content / Caption）、IsReadOnly（别名 ReadOnly）、Multiline、MaxLength、Placeholder（别名 Hint / CueBanner）；删掉 ownProperties + setOwnProperty + getOwnProperty 三段手写 |
| `XmlComboBox.h/.cpp` | 属性表：Items（别名 Strings / Choices）、Text（别名 Content）、SelectedIndex（别名 CurSel）、DropDownStyle（Enum `DropDownList\|DropDown`）；保留 fillItems / 重建行为 |
| `XmlTableView.h/.cpp` | 属性表：Columns、Rows、SelectedRow（别名 CurrentIndex / Index）、RowCount（只读）；删掉把 `Name` 当控件属性的错误声明 |
| `PyMfcXml\PyMfcLoad.cpp` | 两条加载路径（XmlElement 树 / XML dict）的"三种事件 if 链"都换成按契约表遍历 + `BindHostEvent` 统一分派 |

### 行为变化（用户可见）

- XAML 里写 `CheckedChanged=` / `DoubleClick=` / `ValueChanged=` / `DropDown=` / `MouseDown/MouseUp/MouseMove=` / `KeyDown/KeyUp=` / `GotFocus/LostFocus=` 从"声明了没人接"变成真绑定；
  其中前 7 种走宿主专门的注册口，鼠标/键盘/焦点走 `onControlEvent`（键名与宿主派发一致：mousedown/mouseup/mousemove/keydown/keyup/gotfocus/lostfocus）。
- `Scroll=` 宿主目前没有派发能力：加载期明确告警后跳过，不再静默失效。
- 属性名不再大小写/别名敏感地各写一份：`Caption`、`Hint`、`CurSel` 这类历史写法现在与主名等价。
- 声明了但控件不认识的属性（布局属性除外）会进诊断表；`PYARX_PROP_DIAG=1` 时同时落 `log\xml_prop_diag.log`，默认不打日志避免刷屏。

### 验证

- 编译：`MfcXmlCore` + `PyMfcXml` Release x64 通过。
  解释器必须是 free-threaded 3.15t 的 `G:\PyArx\X64\Release\python.exe`；GIL 版或旧版解释器加载不了按 3.15t 编译的 pyd。
- 冒烟 `tests\mfc_xml\_smoke_proptable.py`：10 项全通过——别名 `Caption`/`IsReadOnly` 加载生效、`SelectedIndex` 回填文字、`Rows` 行数正确、`DoubleClick="不存在函数"` 现在报错（证明非 Click 事件进了绑定）、`Scroll="不存在函数"` 只告警不报错、未知属性 `FooBar` 落进诊断日志。
- 模拟消息 `tests\mfc_xml\_sim_msgs.py`：15/15 通过——不发真实鼠标键盘，直接把 `WM_COMMAND(BN_CLICKED / BN_DOUBLECLICKED / EN_CHANGE / CBN_SELCHANGE / CBN_DROPDOWN)`、`WM_HSCROLL`、`WM_KEYDOWN/UP`、`WM_SETFOCUS`、`WM_LBUTTONDOWN/UP` 投给宿主与控件。
  结论：Click / DoubleClick / TextChanged / SelectionChanged / DropDown / CheckedChanged / ValueChanged / KeyDown / KeyUp / GotFocus / MouseDown / MouseUp 十二条通路全部真回调；
  其中 DoubleClick / DropDown / CheckedChanged / 鼠标 / 键盘 / 焦点这 9 条在改造前根本绑不上（加载器只认 Click/TextChanged/SelectionChanged）。
  注意两点：`WM_COMMAND` 要发给宿主的 `PyMfcNativeWindow`（id=0 的子窗口），发给外层框架无效；`CBN_DROPDOWN` 的值是 7（不是 5）。- 回归 `pp_test_all.py`：22 / 27。5 项失败属 app 侧断言与当前实现漂移，与本次封装改动无关，证据是文案已变：测试期望"上游不能是节点自己"，实际输出"第 3 行：起点不能是终点自己"；另外 `文档示例 节点 10221`、`导入 CSV 节点 2` 是示例/CSV 解析的期望值过期。后续若要让回归全绿，应改测试期望而不是回退控件改动。

### 还没做（按计划留给下一刀）

- S3 宿主瘦身：`getXmlValue` / `setXmlValue` 仍是宿主自己的固定 switch（`text` / `curSel` / `rowCount` …），没有把读写转发给控件属性表；因此**运行时改属性只改 XmlRuntime 的元素树**，改不到控件对象。这是"属性三处定义"里剩下的最后一处。
- `xmlEventSchemaJson()` 已能生成 schema，但还没绑到 Python，`MfcXmlRuntime\runtime.py` 的 `EVENT_SPEC` / `_TAG_SPEC` 仍各自维护，下一步让它直接读 schema。
- `Scroll` 派发需要宿主补一条通道；补完后把 `XmlEventInfo` 里该项的 `hostDispatch` 置 true 即可，加载器无需再改。
- 51 个控件里还有 48 个走旧路径（`ownProperties` + `SetOwnProperty`）。基类已经兼容两条路径，可以按"改一个、编一次"的节奏逐个迁，不必成批。

### 追加：ScrollBar 交互补全（2026-11-13）

- 现状核对：独立 `<XScrollBar>`（`kSkinBoxScrollBar = 8`）本来就支持"点轨道跳一页 / 拖拇指 / 滚轮半页"（`PyMfcHost.cpp` 的 `skinBoxMouse`），
  但三者都只走 `setScrollValueInternal`——只改 `m_scrollVal` 再重绘，**一次都不回调 Python**，表现为"能滚但程序不知道"。
- 修法：新增 `setScrollValueUser`（交互专用），值真正变化时才 `invokePython(m_value[id])`；
  程序赋值仍走 `setScrollBarValue`，避免"Python 设值 → 回调 → 再设值"自激；夹边/同值不发事件，滚到底不会刷回调。
- 验证 `tests\mfc_xml\_sim_scrollbar.py`（模拟消息，5/5）：点右轨道 0→10 且回调 1 次、点左轨道回 0、拖拇指到 64、滚轮到 69，共 4 次回调。
- 仍未统一的一处：`ScrollView` 的内容区滚动（`scrollAreaMouse`）点轨道只 `nudgeScroll(±1)`（一次一行，不是一页），且同样没有回调；
  它属于 `Scroll` 事件与 S3 宿主瘦身的范围，本次未动。
### 统一：滚动区域与独立 ScrollBar 对齐（2026-11-13）

对齐前的不一致：`ScrollView` 点轨道走 `nudgeScroll(±1)`（一档 ≈ 量程 1/12，观感约一行），独立 `<XScrollBar>` 点轨道是翻一页；且两者改值都不发事件。

- **点轨道 = 翻一页**：滚动区域改为 `pos ± 视口高度`（`cr.bottom - cr.top`），与独立 ScrollBar 的"跳一页"一致；滚轮仍是一档（`量程/12`），保住手感。
- **位置唯一写入口**：新增 `PyMfcHost::setScrollAreaPos(child, pos, notify)`，夹边、值真变了才重绘；`notify` 时 `fireControlEvent(id, L"scroll")`。`nudgeScroll` 与拖拇指都改走它。
- **事件契约**：`XmlEvents.h` 里 `Scroll` 的 `hostDispatch` 由 `false` 改 `true`，加载器无需改动即按 `onControlEvent(id, "scroll")` 绑定；独立 ScrollBar 的 `setScrollValueUser` 除 `ValueChanged` 外也发一次 `"scroll"`。
- **行为变化**：`Scroll=` 引用不存在的回调现在与其它事件一样在加载期报错（以前只告警、静默不绑）。

验证（全部模拟消息，不碰真实输入设备）：
- `tests\mfc_xml\_sim_scrolled.py` 7/7：量程 0~600、视口 200，点轨道 0→200→400、点上半区回 200、拖拇指到 526、滚轮到 576，`Scroll` 回调 5 次。
- 回归 `_smoke_proptable.py` 10/10（Scroll 断言改为"缺回调报错/有回调正常绑定"）、`_sim_msgs.py` 15/15、`_sim_scrollbar.py` 5/5。
### 追加：悬浮滚轮（2026-11-13）

问题：`WM_MOUSEWHEEL` 只投给焦点窗口，滚动条没有焦点时滚不动（Windows 缺省不按光标位置投递）。

做法：复用宿主已有的线程消息钩子（原 Tab 用的 `WH_GETMESSAGE`）新增滚轮分支——取 `lParam` 里的屏幕坐标 →
`WindowFromPoint` → 沿父链找挂了 `PyMfcSkinHost` 属性的窗口 → 交给宿主 `wheelOnHoveredWindow`：

- 独立 `ScrollBar`：整块可滚，一步 = `PageSize/2`（最少 1），走 `setScrollValueUser`，`ValueChanged` 与 `scroll` 都发。
- `ScrollView`：只认右侧 18px 滚动条条带，一步 = 量程 1/12（与内容区滚轮一致），走 `nudgeScroll → setScrollAreaPos`，发 `scroll`。
- 命中非滚动件：不消费，交回原流程，内容控件自己的滚轮行为一点没变。

验证 `tests\mfc_xml\_sim_wheel.py` 7/7（`PostMessage` 投递 `WM_MOUSEWHEEL`，坐标写在 `lParam` 里，不动真实鼠标）：
焦点停在 TextBox 时，悬浮 ScrollBar 下滚 0→5、上滚 5→0 且回调；悬浮滚动区域条带 0→50 且 `Scroll` 回调；
光标落在普通控件上时两者都不动。
回归：`_sim_scrolled.py` 7/7、`_sim_scrollbar.py` 5/5、`_sim_msgs.py` 15/15、`_smoke_proptable.py` 10/10。
### 追加：悬浮光标（2026-11-13）

- 手型（`IDC_HAND`）：真控件 hover——`Button/ToggleButton/BitmapButton`、`Hyperlink`（SysLink）、
  `ComboBox`、皮肤件里的 `ScrollBar/Switch/SpinBox/RangeSlider/PageIndicator/Dial/Tumbler/Table/Tree/ListView/GridView/PathView/Menu/MenuBar`，
  以及任何绑了 `Click`/`DoubleClick` 的卡片类控件；`ScrollView` 只在右侧 18px 滚动条条带上给手型。
- 爪型：`QCanvas` / `Flickable` / `DwgCanvas`。系统光标表里没有 grab 手型（`C:\Windows\Cursors` 只有
  aero_arrow / aero_link / aero_move 等，Win32 光标 ID 也无 grab），所以用资源文件 `res\grab.cur`：
  **当前版本是直接从浏览器实际使用的 HCURSOR 抓出来的原图**（见文末"直接从浏览器提取光标"一节），
  造型、描边粗细与浏览器 `cursor:grab` 完全一致；只为对齐已认可的观感，把手形高度缩到画布 32px 的 58%
  （`FILL=0.58`，热区 (17,17)）。按下拖动时换 `res\grab_down.cur`（浏览器 `cursor:grabbing` 的握拳图，FILL 同比例，热区 (16,15)）。
  早期用截图 + 自绘重建（`docs\_grab_grid.txt` → `_make_grab_cur.py` 的 flood fill / 对比度拉伸 / Zhang-Suen 细化）
  的版本已被用户否掉三次（"太丑"、"画的不像"），只保留脚本作历史，不再作为发布资源。
  踩坑：CUR 里放 48+32 两个尺寸时，`LoadImageW(...,0,0)` 取的是排在前面的 48 那张，Windows 按 48 原生像素
  绘制，光标看起来永远偏大——所以只保留 32 一张。
  自绘路线（胶囊 SDF / polygon）已全部废弃，不再使用 `_gen_grab_cursor.py`。
  `pyMfcGrabCursor()` 从 `<pyd 目录>\res\grab.cur` 载入，载不到退回手型（绝不退回箭头）。
- 容器/空白（`Pane`、`Pager`、`ToolBar`、`BusyIndicator`、`Shadow`、`Label`）仍是箭头；`Edit/RichText` 沿用系统的 I 型。
- 复位：`WM_MOUSELEAVE` 把光标复位成箭头，不会出现"手型挂在窗外"。
- 部署：`res\grab.cur` 必须跟 pyd 一起发布（已复制到 `X64\Release\res\grab.cur`）。

验证（`SendMessage(WM_SETCURSOR)` + `GetCursor()` 对比句柄，不动真实鼠标）：
`tests\mfc_xml\_sim_cursor.py` 8/8——按钮/Toggle/超链接/ScrollBar 均为 `IDC_HAND`，label 与 Panel 空白为箭头，
QCanvas 句柄既非手型也非箭头（`res\grab.cur` 载入的爪型），`WM_MOUSELEAVE` 后复位箭头；
`tests\mfc_xml\_sim_cursor_plan.py` 1/1——DwgCanvas 同为资源爪型。
回归：`_sim_wheel.py` 7/7、`_sim_scrolled.py` 7/7、`_sim_scrollbar.py` 5/5、`_sim_msgs.py` 15/15、`_smoke_proptable.py` 10/10。

### 追加：直接从浏览器提取光标（2026-11-13）

用户要求"直接调用浏览器自绘的鼠标图例"、"强制提取浏览器自带的所有鼠标图标"，因此不再按截图重画。

**三条走不通的路（都已验证并放弃）**

1. PE 资源表：`msedge.exe` / `msedge.dll` 里 `RT_CURSOR`、`RT_GROUP_CURSOR` 均为 0；同时 `_extract_browser_cursors.py`
   的 `walk()` 解析出的条目 type 是乱码，说明该脚本本身有 bug，其 "0 个光标" 结论不可信——但改用下面的
   HCURSOR 抓取后不再需要修它。
2. `.pak` 内嵌 `.cur`：`tests\mfc_xml\_scan_pak_cursors.py` 按 ICONDIR 签名（`00 00 02 00` + 条目数 + 条目合法性）
   扫 Edge 的 `resources.pak`(7362 条)、`msedge_100_percent.pak`(407 条)、`msedge_200_percent.pak`(407 条)，
   命中 0。
3. pak 里的小 PNG：`_pak_dump.py` 导出的 116 / 256 张全是 favicon 与 UI 图标，`docs\_pak_sheet.png` 目视确认无手形。

**可行路线：抓浏览器正在用的 HCURSOR**

Windows 版 Chromium 的 `cursor:grab` / `cursor:grabbing` 是运行时用内部位图 `CreateCursor` 出来的，
不落成独立 `.cur` 文件，但**句柄在系统里是可读的**：

1. `tests\mfc_xml\_grab_browser_cursor.py` 写一个 9 行 `cursor:grab / grabbing / default / pointer / move /
   not-allowed / zoom-in / text / nesw-resize` 的测试页，用临时 `--user-data-dir` 起 Edge（不碰用户配置）；
2. 沿客户区纵向扫描 `SetCursorPos` + `GetCursorInfo`，按句柄去重（步长 8px），把每个不同句柄都记下来；
3. `GetIconInfo` 取热区，`DrawIconEx` 到 32bpp DIB 导出位图，再写成 `.cur`。

结果：`cursor:grab` = 开放手掌（句柄 `0x3a00e6b3`，热区 13,13），`cursor:grabbing` = 握拳（`0x2a66ae75`，热区 13,13），
两者都**不是**系统 `IDC_HAND`(32649)，也不是箭头(32512)——证明确实是浏览器自绘。

**两个必须记的坑**

- 写 `.cur` 时 ICONDIR 的 `dwBytesInRes` 必须填 **DIB 本身长度**（不含 6+16 头部），写成"含头部"的总长虽然
  长度自洽，但 `LoadImageW` 直接返回 0；`_make_grab_cur.image_bytes()` 一直是这个口径，复用时别改。
- Chromium 光标**没有 alpha 通道**，`GetDIBits` 读 `hbmMask`（1bpp 掩码）到 32bpp 缓冲会拿到垃圾（表现为竖条纹）。
  正确做法是"黑白两底求 alpha"：同一光标画到纯白底与纯黑底各一份，`alpha = 255 - (白底 - 黑底)`，
  颜色 = 黑底像素 / (alpha/255)。

**落地**

- `tests\mfc_xml\_grab_from_browser.py`：读上面抓到的两张 `.cur`，按 `FILL=0.58` 重排到 32×32（3×3 超采样、
  热区同比缩放），写出 `G:\PyArx\res\grab.cur`（4286 B，热区 17,17）与 `res\grab_down.cur`（4286 B，热区 16,15），
  两个都通过 `LoadImageW` 校验。
- 部署副本 `X64\Release\res\grab.cur`、`grab_down.cur` 已同步；`pyMfcGrabCursor()` / `pyMfcGrabDownCursor()`
  按 `<pyd 目录>\res\` 查找，缺 `grab_down.cur` 时退回 `grab.cur`。
- 既然拿到了真图，`docs\_browser_cursors\` 里 8 种浏览器光标（箭头/工字/开放手/握拳/指向手/移动/禁止/放大）
  都已存成 PNG + CUR，后续要别的形状直接换文件即可。

**验证**

- `tests\mfc_xml\_sim_cursor.py` 8/8、`_sim_cursor_plan.py` 1/1（QCanvas / DwgCanvas 句柄既非箭头也非 `IDC_HAND`）。
- 新增 `tests\mfc_xml\_probe_plan_cursor.py`：在真实运行的配电图应用上移动鼠标并按住左键，
  读 `GetCursorInfo`——悬浮 `0xb641e65`（grab）、按下与拖动 `0x4a6f1471`（grabbing，与悬浮不同）、
  松开回 `0xb641e65`，三项检查全过。
- `docs\_grab_size_cmp.png`：系统箭头 / 系统手型 / 新 grab 三者 32px×8 倍并排，新 grab 比系统手型更小、
  描边是浏览器原图的抗锯齿细边。

### 追加：第二轮审计 —— S1/S2 落地后的真实覆盖率（2026-11-13）

沿用首轮 S1~S5 的口径（属性归控件、事件归契约、复合行为归 XAML/宿主最薄处），这一轮只量"改完之后还剩多少"，
不重复首轮已结论。测量脚本 `tests\mfc_xml\_audit_contracts.py`（事件契约 vs 控件短名 vs 注册标签）、
`tests\mfc_xml\_audit_props.py`（属性声明 vs 实现分支），两者都是纯文本静态扫描，可重跑。

#### 覆盖率表

| 维度 | 数量 | 说明 |
|---|---|---|
| 控件头文件 | 61 | `MfcXmlCore\controls\Xml*.h` |
| 控件类（有 type() 短名） | 48 | 其余是基类/工具头（XmlTypes / XmlEvents / XmlAppearance / XmlContentParse / XmlMfcHost …） |
| 注册标签（`RegisterOne`） | 54 | 每次调用可带 1~4 个别名 |
| **已迁到 `ownPropTable`** | **3 / 48** | 只有 TextBox、ComboBox、TableView |
| 仍走旧三钩子（`ownProperties`+`setOwnProperty`+`getOwnProperty`） | 40 | 属性名、别名、默认值仍各写一遍，S1 只动了样板 |
| 事件契约覆盖 | 48 / 48 | `xmlDefaultEvents(m_type)` 全部命中，0 个控件无契约；但也 0 个控件 override，即全吃类型默认表 |
| `IXmlMfcHost` 虚函数 | 159 | 与首轮一致，S1/S2 未削减 |
| `PyMfcHost.cpp` / `PyMfcProcs.cpp` / `PyMfcLoad.cpp` | 6241 / 2432 / 1506 行 | 宿主仍是最大的编译单元 |
| `kSkinBox` 在 `PyMfcHost.cpp` 出现 | 62 次 | 自绘件的绘制 + 命中 + 光标三套分支都挂在 kind 上 |

结论：**S1 的机制成立了，但只覆盖 3/48；S2 的机制齐了（契约无缺口），但没有任何控件自己申报事件，
"事件归控件"这一步实际上还没开始。** 加一个控件仍然是"注册 + 头 + cpp 三钩子 + 宿主 kind 分支"五处。

#### 缺陷一（P0）：属性表与实现双向漂移，40 个控件里已量出三类

`_audit_props.py` 抓的是"声明"与"分支"不一致，不是风格问题，是行为问题：

1. **表里声明了、set 里没有分支 → 写了静默返回 false**
   - `XmlBusyIndicator.cpp`：`Running`
   - `XmlDwgCanvas.cpp`：`Source`
   XAML 里写 `<XDwgCanvas Source="…">` 从属性接口进去时直接被拒，且因为 `ownProperties` 里声明过，
   调用方会以为"这是合法属性"，比未知属性更难查。
2. **实现认别名、表里没声明 → 别名回读/诊断都看不到**（23 个控件）
   最典型：`XmlScrollBar.cpp`（`page`、`position`）、`XmlRangeSlider.cpp`（`from`、`to`）、
   `XmlSwitch.cpp`（`ischecked`、`on`）、`XmlTumbler.cpp`（`index`、`value`）、
   `XmlListView.cpp` / `XmlGridView.cpp` / `XmlTreeView.cpp` / `XmlToolBar.cpp`（`index`/`itemcount`/`currentindex`）。
   后果是同一属性在"写"上是合法的、在 `properties()` 导出和未知属性诊断里却不存在——
   XAML 设计器/回读接口拿不到这个属性，两边永远对不齐。
3. **三件套本身仍在重复定义**：`XmlSpinBox.cpp` 是完整样本——`ownProperties` 声明
   `Name / Value / Minimum / Maximum`，`setOwnProperty` 认 `value / min / max`（**没有 name**），
   `getOwnProperty` 同样。于是 `Minimum` 只能靠别名 `min` 生效、`Name` 表里有而写不进。
   这正是首轮 P0-3"属性三处手写且已漂移"的原样复现，只是漂移点从控件间搬到了控件内的三个函数之间。

#### 缺陷二（P1）：一个控件三个名字——type() / XAML 标签 / 契约 key 互相对不上

`xmlDefaultEvents(m_type)`、诊断日志、`RegisterOne` 的标签是三套命名，当前有 4 个控件对不上：

| 控件类 | `type()` | 实际注册标签 |
|---|---|---|
| `XmlTabControl` | `tab` | `TabBar` / `tabcontrol` / `TabWidget` / `tabwidget`（无 `Tab`） |
| `XmlToggleButton` | `togglebutton` | `ToggleButton` / `togglebutton` / `ToggleBtn`（无 `Toggle`/`toggle`） |
| `XmlToolBar` | `toolbar` | `ToolBar` / `toolstrip` / `ToolButtonBar`（`ToolBar` 归布局层） |
| `XmlMenuBar` | `menubar` | `MenuBar` / `menustrip` / `MenuBarControl` / `TopMenuBar` |

后两者是为避免抢布局层的旧别名（`ToolBar`/`TreeView`）而特意改名的，属于**有意分叉但没留说明**；
前两者是真缺口：XAML 里写 `<Toggle …>`、`<Tab …>` 不成立，而契约表和诊断里这两个控件就叫 `toggle`、`tab`。
要么统一"标签集合是 type() 的超集"，要么给 `type()` 加一个"契约名"字段，两者只留一个来源。

#### 缺陷三（P1）：kind 用魔数跨编译单元传播

`XmlSpinBox.cpp` 的 `createNative` 写的是 `host->addSkinBox(id, 2, x, y, w, h)`——字面量 `2`。
宿主的 `kSkinBoxSpin` 是 `PyMfcHost.h:58` 的枚举，值一旦重排，编译不会报错、运行时画错东西。
同类风险还有：`paintSkinControl`（62 处 `kSkinBox`）、`skinBoxMouse`、`hoverCursor` 都把行为按 kind 分支，
新增一个自绘件要同步改 5 处；`hoverCursor` 里还硬编码了 `cr.right - 18`（滚动条热区宽度）和
`BUTTON` / `SysLink` / `ComboBox` 三个类名字符串。

#### 缺陷四（P1，仍未定位）：`pp_event_test.py` / `pp_grid_test.py` 的 GIL 崩溃

本轮把嫌疑范围又缩了一圈：

- `PyMfcCommon.h:486 invokePython` 内部**已有** `py::gil_scoped_acquire`，且 `callback` 为空时直接返回；
- 派发侧 `OnCommandRange` / `OnDblClickRange` / `OnComboDropDown` / `OnHScroll` / `OnNotify` 都只是
  `map.find` + `invokePython(it->second)`，`it->second` 是引用，不产生 `py::object` 拷贝；
- `hoverCursor`(`PyMfcHost.cpp:1569`) 只对 `m_click`/`m_dblclick` 做 `.count()`，不拷贝对象；
- `pymfcDir()`(`PyMfcCommon.h:406`) 是纯 `GetModuleFileNameW`，不碰 Python。

因此"WM_SETCURSOR 新路径导致崩溃"这一假设**证据不足**，应当排除。剩下两个待证方向：

1. 某处 `py::object` 的拷贝/析构不在 GIL 内——pybind11 报的正是 `inc_ref() … on a method object`，
   而 `pp_app` 里 `child.click = fn` 存的就是 bound method；
2. ctypes `SendMessageW` 期间 GIL 被释放，宿主过程里若触发 Python 对象生命周期（含 Python GC 恰好回收
   一个 bound method），就会撞在这条断言上。

定位手段（下次动这块时照做，不要盲改）：
- 用 `PYARX_MFC_DEBUG=1` 跑 `pp_event_test.py`，看 `[callback]` 行最后一条落在哪个控件；
- 在 `invokePython` 里加 `PyGILState_Check()` 断言 + `debugLogf`，确认崩溃点是否真在回调内；
- 若断言通过仍崩，则把嫌疑转到 `m_*` map 的增删（`bindEvent`/`clearHandlers` 在无 GIL 路径被调用）。

#### 下一轮建议（按收益/成本）

1. **把 23 个别名回填成表声明**（纯加行，零行为变化）：让 `properties()` 与实现对齐，顺带消掉
   "写合法、读不到"的错位。2 个空声明（`Running` / `Source`）补分支或从表里删掉。
2. **`XmlSpinBox` 这类三钩子控件批量迁表**：`XmlPropEntry` 已经有 `Name/别名/类型/Set/Get`，
   迁移是机械替换；先迁"取值班"（Slider、RangeSlider、ScrollBar、SpinBox、ProgressBar、Dial、Tumbler）6~7 个，
   它们属性少、测试齐（`_sim_scrollbar.py` / `_sim_scrolled.py` 可复用）。
3. **`type()` 加 `contractName()`**：只改基类一个虚函数 + 4 个控件，标签集合不必动，立刻消除缺陷二。
4. **kind 常量下沉**：`addSkinBox(id, 2, …)` 改成 `addSkinBox(id, XmlSkin::Spin, …)`，
   把枚举放 `XmlTypes.h`，宿主 include 同一个头——一处定义、两处使用。
5. 复合控件空心化（首轮 P0-4）依旧成立：`kSkinBox` 62 处证明自绘件的绘制/命中/光标逻辑都还在宿主里，
   这条要等上面 1~4 做完再动，顺序不能倒。
---

## 追加：属性规范名统一（去别名）

约束：一个属性只保留一个规范名，别名分支全部删除，不再为旧写法回填。落实位置：

- **实现层**：25 个控件 cpp 的 `xmlprop::StrEq` 别名链压成单条规范分支；属性表第二字段（Aliases）全部清空为 `nullptr`；`XmlProperty.aliases` 字段与其传递一并回退。
- **声明层**：重复声明合并（CodeEditor/ToolTip 的 Content+Text、PageView 的 CurrentIndex+Index、PageIndicator/PathView/TreeView 的 CurrentIndex/Index/SelectedRow 统一为 SelectedIndex）。
- **解析层**：`XmlCore` 的语义文字取 `Text → Content(Popup 正文) → Header`；`PyMfcLoad` 建控件时同样按此顺序，没有再以 Content 为第一优先。
- **设计器**：`UIDesign` 新建控件写 `Text`，画面/属性面板读 `Text`（GroupBox 用 `Header`），代码折叠按钮改写 `Text`。
- **仓库用法**：`tests`、`examples`、`UIDesign` 下 36 个文件、100 处属性写法已同步（脚本 `tests\mfc_xml\_unify_usage.py`，可按标签-属性映射重跑）。

规范名清单（其余一律视为错误写法）：

| 概念 | 规范名 | 被删除的旧写法 |
|---|---|---|
| 显示文字 | Text | Content、Caption、Header（TextBlock/GroupBox 混用） |
| 容器标题 | Header（仅 GroupBox） | Content、Text |
| 勾选状态 | Checked | IsChecked、On |
| 数值 | Value | Position |
| 取值范围 | Minimum / Maximum | Min、Max；Dial 的 From/To |
| 区间滑块 | LowValue / HighValue | From、To |
| 选中项 | SelectedIndex | Index、CurrentIndex、SelectedRow、CurSel |
| 分页滚动 | PageSize | Page |
| 列表内容 | Items | Strings、Choices |
| 图片 | Image | Source、Src |
| 链接 | Url | Href、NavigateUrl |
| 只读 | IsReadOnly | ReadOnly |
| 滚动范围（ScrolledWindow） | ScrollMin / ScrollMax | Minimum / Maximum |
| 计数 | Count | ModelCount |

验证：`_smoke_proptable.py` 10/10、`_sim_msgs.py` 15/15、`pp_test_all.py` 27/27；配电图应用截图正常（39 节点、工具条与表头文字完整）。`UIDesign` 源文件 `ClCompile` 无错误（完整链接阶段受本机提权限制未跑，属既有环境问题）。
## 追加：事件契约名与注册标签对齐

第二轮审计里"一个控件三个名字"的修法：**契约名 = 注册主标签**，`type()` 不再自造短名。

- 12 个控件改了契约名：`combo→ComboBox`、`group→GroupBox`、`progress→ProgressBar`、`radio→RadioButton`、`tab→TabControl`、`label→TextBlock`、`toggle→ToggleButton`、`toolbar→ToolStrip`、`menubar→MenuStrip`、`tree→TreeCtrl`、`treeview→TreeTable`、`calendar→CalendarCtrl`。
- `XmlEvents.h` 的默认事件表从 51 条混写（含 `edit/combo/radio/group/label/progress/tree/tab/toolbar/menubar/pageview/static/maskedtextbox` 等别名键）收敛为 49 条，一控件一键；查找仍大小写不敏感。
- 校验脚本 `tests\mfc_xml\_audit_contract_keys.py`：49 个控件的契约名全部能在表里命中，表内无孤立键（`page/stackview/swipeview` 是 `XmlPageView` 按标签变化的三型）。
- 回归：`_smoke_proptable` 10/10、`_sim_msgs` 15/15、`pp_test_all` 27/27。

## 追加：标签别名现状（未动，需先解两层注册）

注册表 `RegisterPyMfcControls.cpp` 仍有 54 条标签、大量别名，其中一部分**不能直接删**：`TreeView / ToolBar / MenuBar / ListView / StatusBar / PropertyGrid` 同时属于原生 MFC 标签层（`XmlMfcTags.h`）与设计器控件箱，删掉自绘侧的别名会让同一标签指向另一套实现。仓库实测使用量（`_audit_tags.py`）：`Label=5、Edit=6、Progress=8、DrawCanvas=5、PaintCanvas=3、TreeView=2、ListView=1`，其余别名一律 0 次。

结论：标签去别名必须先把"自绘控件注册"与"原生 MFC 标签"拆成两个命名空间（或给自绘控件统一加前缀），否则改一个名字会静默切换实现。这是复合控件空心化之后的下一个结构性动作，未在本次改动中执行。
## 属性表迁移收口（S2）

39 个控件已全部改为声明式属性表（`XmlPropEntry`），旧三钩子 `ownProperties / setOwnProperty / getOwnProperty` 归零，`_audit_prop_migration.py` 输出 0 项待迁移。迁移约定：属性只在表里声明一次（名字/类型/枚举/读写），值在 attach 前落成员、在 `createNative` 里下发；`Set == nullptr` 即只读；`Name` 不再作为自有属性（由基类布局属性处理）。

统一导出：`xmlPropertySchemaJson()`（C++）/ `PyMfcXml.prop_schema()`（JSON 字符串），设计器属性面板与 Python 内省共用一份 schema。顺带修掉基类 `properties()` 把 `writable` 填进 `readable` 槽位的缺陷——此前只读属性在面板里可编辑。