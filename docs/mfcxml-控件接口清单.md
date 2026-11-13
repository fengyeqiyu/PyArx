# MFCXaml 控件接口清单

> 数据来源：运行中的 `PyMfcXaml.prop_schema()`（`XamlPropertySchemaJson()` 导出）+ `MfcXamlCore\controls\RegisterPyMfcControls.cpp`（标签注册）+ `XamlEvents.h`（事件契约）。
> 属性内容就是控件真正读写的字段，不是文档承诺。schema 变了重跑本文档：`python tools\gen_control_doc.py`（需先构建出 `X64\Release\PyMfcXaml.pyd`）。

## 一、读数口径

- **通用属性 19 条**：每个控件都继承自 `XamlMfcControlBase`，是布局/外观字段 （`Shape / BackColor / FrontColor / Foreground / BorderColor / BorderStyle / BorderThickness / Radius / Opacity / TextAlign / VerticalAlignment / TextWrapping / FontWeight / FontStyle / FontSize / FontFamily / Level / TabIndex / Theme`）。它们不构成"封装程度"。
- **自有属性**：该控件自己声明、真正对应业务语义的字段（如 `Text`、`Items`、`Value`、`Columns`），本文档的"档位"只看这一列。
- **档位**：`S 完整` 自有属性 ≥5 · `A 基本` 3–4 · `B 薄` 1–2 · `C 仅布局` 0。
- **只读属性**：`Set == nullptr`，只能 `get`（如 `RowCount`、`ItemCount`、`PageCount`）。

## 二、总览

| 项 | 数量 |
|---|---|
| 控件（注册主标签） | 62 |
| 属性条目合计（含通用） | 1310 |
| 其中通用属性 | 1179（62 控件 × 19） |
| 其中自有属性 | 131 |
| 只读属性 | 7 |
| 事件键（内部名） | 15（click / dblclick / change / select / value / checked / dropdown / scroll / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus） |

档位分布：S 完整 **3** 个 · A 基本 **18** 个 · B 薄 **37** 个 · C 仅布局 **4** 个 · 

## 三、逐控件清单

| 控件 | 实现 | 自有属性（读写） | 只读 | 事件 | 档位 | 源文件 |
|---|---|---|---|---|---|---|
| **XButton** | 原生 MFC Button | Text | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlButton.cpp` |
| **XLabel** | 原生 MFC Static | Text | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlTextBlock.cpp` |
| **XTextField** | 原生 MFC Edit | Text / IsReadOnly / Multiline / MaxLength / Placeholder | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | S 完整 | `XamlTextBox.cpp` |
| **XListBox** | 原生 MFC ListBox | Items / SelectedIndex / SelectionMode | — | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlListBox.cpp` |
| **XCodeEditor** | C++ 自绘编辑器 | Text / Syntax / IsReadOnly | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | A 基本 | `XamlCodeEditor.cpp` |
| **XCheckBox** | 原生 MFC Button(BS_AUTOCHECKBOX) | Text / Checked | — | click / checked / dblclick / gotfocus / lostfocus | B 薄 | `XamlCheckBox.cpp` |
| **XRadioButton** | 原生 MFC Button(BS_AUTORADIO) | Text / Checked | — | click / checked / gotfocus / lostfocus | B 薄 | `XamlRadioButton.cpp` |
| **XGroupBox** | 原生 MFC Button(BS_GROUPBOX) | Header | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlGroupBox.cpp` |
| **XComboBox** | 原生 MFC ComboBox | Items / Text / SelectedIndex / DropDownStyle | — | select / change / dropdown / dblclick / gotfocus / lostfocus | A 基本 | `XamlComboBox.cpp` |
| **XProgressBar** | 原生 MFC msctls_progress32 | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlProgressBar.cpp` |
| **XSlider** | 原生 MFC msctls_trackbar32 | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlSlider.cpp` |
| **XTabBar** | 原生 MFC SysTabControl32 | Pages | — | click / select / gotfocus / lostfocus | B 薄 | `XamlTabControl.cpp` |
| **XTreeView** | 原生 MFC SysTreeView32 | Nodes | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlTreeCtrl.cpp` |
| **XListCtrl** | 原生 MFC SysListView32 | Columns / Rows | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlListCtrl.cpp` |
| **XChoice** | 原生 MFC ComboBox(CBS_DROPDOWNLIST) | Items | — | select / dropdown / dblclick / gotfocus / lostfocus | B 薄 | `XamlChoice.cpp` |
| **XDatePicker** | 原生 MFC SysDateTimePick32 | Value | — | select / value / dblclick / gotfocus / lostfocus | B 薄 | `XamlDatePicker.cpp` |
| **XPane** | 原生 MFC Static 容器 | — | — | click / dblclick / mousedown / mouseup / mousemove | C 仅布局 | `XamlPanel.cpp` |
| **XSplitView** | 原生 MFC 自注册拖动条 | — | — | mousedown / mouseup / mousemove / dblclick | C 仅布局 | `XamlSplitter.cpp` |
| **XDivider** | 原生 MFC Static(SS_ETCHEDHORZ) | — | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | C 仅布局 | `XamlStaticLine.cpp` |
| **XPowerCanvas** | 宿主自绘画布（配电规划图） | Data | — | click / dblclick / mousedown / mouseup / mousemove / scroll / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlPowerCanvas.cpp` |
| **XHyperlink** | 原生 MFC SysLink | Text / Url | — | click / dblclick / gotfocus / lostfocus | B 薄 | `XamlHyperlink.cpp` |
| **XTextArea** | 原生 MFC RICHEDIT50W | Text / Bold / Italic | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | A 基本 | `XamlRichText.cpp` |
| **XToggleButton** | 原生 MFC Button(BS_AUTOCHECKBOX 自绘) | Text / Checked | — | click / checked / dblclick / gotfocus / lostfocus | B 薄 | `XamlToggleButton.cpp` |
| **XIconButton** | 原生 MFC Button(BS_BITMAP) | Text / Image | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlBitmapButton.cpp` |
| **XImage** | 原生 MFC Static(SS_BITMAP) | Image | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlStaticBitmap.cpp` |
| **XGridLayout** | 原生 MFC 自注册网格 | Columns / Rows | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlGrid.cpp` |
| **XScrollView** | 原生 MFC 自注册滚动容器 | ScrollMin / ScrollMax | — | scroll / click / gotfocus / lostfocus | B 薄 | `XamlScrolledWindow.cpp` |
| **XMonthGrid** | 原生 MFC SysMonthCal32 | Value | — | select / value / dblclick / gotfocus / lostfocus | B 薄 | `XamlCalendarCtrl.cpp` |
| **XSwitch** | 自绘 skin #1 Switch | Checked | — | click / checked / value / gotfocus / lostfocus | B 薄 | `XamlSwitch.cpp` |
| **XSpinBox** | 自绘 skin #2 Spin | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlSpinBox.cpp` |
| **XRangeSlider** | 自绘 skin #3 Range | LowValue / HighValue / Minimum / Maximum | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlRangeSlider.cpp` |
| **XBusyIndicator** | 自绘 skin #4 Busy | — | — | click / gotfocus / lostfocus | C 仅布局 | `XamlBusyIndicator.cpp` |
| **XPageIndicator** | 自绘 skin #5 Pages | Count / SelectedIndex | — | click / select / gotfocus / lostfocus | B 薄 | `XamlPageIndicator.cpp` |
| **XDial** | 自绘 skin #6 Dial | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlDial.cpp` |
| **XTumbler** | 自绘 skin #7 Tumbler | Count / SelectedIndex | — | value / scroll / gotfocus / lostfocus / click | B 薄 | `XamlTumbler.cpp` |
| **XScrollBar** | 自绘 skin #8 ScrollBar | Minimum / Maximum / Value / PageSize / Orientation | — | value / scroll / gotfocus / lostfocus | S 完整 | `XamlScrollBar.cpp` |
| **XTableView** | 自绘 skin #9 Table（独立控件套装） | Columns / Rows / SelectedIndex / RowCount | RowCount | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlTableView.cpp` |
| **XTreeTable** | 自绘 skin #10 Tree | Nodes / SelectedIndex | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlTreeView.cpp` |
| **XToolBar** | 自绘 skin #11 ToolBar | Items / SelectedIndex | — | click / select / dropdown | B 薄 | `XamlToolBar.cpp` |
| **XToolTip** | 自绘 skin #12 ToolTip | Text | — | click | B 薄 | `XamlToolTip.cpp` |
| **XMenu** | 自绘 skin #13 Menu | Items / SelectedIndex | SelectedIndex | click / select | B 薄 | `XamlMenu.cpp` |
| **XMenuBar** | 自绘 skin #14 MenuBar | Items / SelectedIndex | — | click / select | B 薄 | `XamlMenuBar.cpp` |
| **XPopup** | 自绘 skin #15 Popup | Title / Content / Lines | — | click / gotfocus / lostfocus | A 基本 | `XamlPopup.cpp` |
| **XVirtualList** | 自绘 skin #16 ListView（虚拟化） | Items / ItemHeight / SelectedIndex / ItemCount | ItemCount | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlListView.cpp` |
| **XStackView** | 自绘 skin #17 Pager（XamlPageView 子类） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlStackView.h` |
| **XSwipeView** | 自绘 skin #17 Pager（XamlPageView 子类） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlSwipeView.h` |
| **XGridView** | 自绘 skin #18 GridView | Items / SelectedIndex / CellWidth / CellHeight / ItemCount | ItemCount | select / click / dblclick / scroll / gotfocus / lostfocus | S 完整 | `XamlGridView.cpp` |
| **XPathView** | 自绘 skin #19 PathView | Items / SelectedIndex | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlPathView.cpp` |
| **XQCanvas** | 自绘 skin #20 QCanvas（命令脚本画布） | Commands / Script | — | click / dblclick / mousedown / mouseup / mousemove / scroll / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlQCanvas.cpp` |
| **XPage** | 自绘 skin #17 Pager（XamlPageView） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlPage.cpp` |
| **XFlickable** | 自绘 skin #21 Flick | OffsetX / OffsetY | — | scroll / click / mousedown / mouseup / mousemove | B 薄 | `XamlFlickable.cpp` |
| **XFrame** | 原生 MFC Panel + 内边距 | Padding | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlFrame.cpp` |
| **XDialog** | 原生 MFC Panel（模态标记） | Title / Modal / StandardButtons | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | A 基本 | `XamlDialog.cpp` |
| **XRoundButton** | 原生 MFC Button（圆形外观） | Text | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlRoundButton.cpp` |
| **XDelayButton** | 原生 MFC Button（长按确认） | Text / Delay / Checked | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | A 基本 | `XamlDelayButton.cpp` |
| **XToolButton** | 原生 MFC Button（图标+文字） | Text / IconSource / Display | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | A 基本 | `XamlToolButton.cpp` |
| **XToolSeparator** | 原生 MFC StaticLine | Orientation | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlToolSeparator.cpp` |
| **XTabButton** | 原生 MFC Button（页签态） | Text / Checked | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlTabButton.cpp` |
| **XScrollIndicator** | 自绘细条指示器 | Orientation / Size / Position | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | A 基本 | `XamlScrollIndicator.cpp` |
| **XMenuItem** | 原生 MFC Button（菜单项） | Text / Checkable / Checked | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | A 基本 | `XamlMenuItem.cpp` |
| **XMenuSeparator** | 原生 MFC StaticLine | Orientation | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlMenuSeparator.cpp` |
| **XItemView** | 属性驱动行模板容器（Model 逐行实例化 delegate 子节点） | Model / ItemHeight / SelectedIndex | — | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlItemView.cpp` |

## 四、分档结论

**S 完整（自有 ≥5，可直接当业务控件用）**：XTextField(5)、XScrollBar(5)、XGridView(5)

**A 基本（3–4，够用但还缺边角能力）**：XListBox(3)、XCodeEditor(3)、XComboBox(4)、XProgressBar(3)、XSlider(3)、XTextArea(3)、XSpinBox(3)、XRangeSlider(4)、XDial(3)、XTableView(4)、XPopup(3)、XVirtualList(4)、XDialog(3)、XDelayButton(3)、XToolButton(3)、XScrollIndicator(3)、XMenuItem(3)、XItemView(3)

**B 薄（1–2，只有主语义那一个字段）**：XButton(1)、XLabel(1)、XCheckBox(2)、XRadioButton(2)、XGroupBox(1)、XTabBar(1)、XTreeView(1)、XListCtrl(2)、XChoice(1)、XDatePicker(1)、XPowerCanvas(1)、XHyperlink(2)、XToggleButton(2)、XIconButton(2)、XImage(1)、XGridLayout(2)、XScrollView(2)、XMonthGrid(1)、XSwitch(1)、XPageIndicator(2)、XTumbler(2)、XTreeTable(2)、XToolBar(2)、XToolTip(1)、XMenu(2)、XMenuBar(2)、XStackView(2)、XSwipeView(2)、XPathView(2)、XQCanvas(2)、XPage(2)、XFlickable(2)、XFrame(1)、XRoundButton(1)、XToolSeparator(1)、XTabButton(2)、XMenuSeparator(1)

**C 仅布局（自有 0，属性面上等同一块带样式的面板）**：XPane、XSplitView、XDivider、XBusyIndicator

C 档不是"没实现"——这些控件在 C++ 里都有真实原生窗口/自绘实现、事件也照常派发，只是**业务字段没有暴露成属性**。要让它们进设计器可配置，得按需要补属性表条目；这也是 `plugins` 层用 XAML 组合补位的典型对象（组合态用子控件属性，不硬造新属性）。

## 五、命名规范

标签统一为 `X + 本意名`（`XButton` / `XTextField` / `XStackPanel` / `XWindow`），只保留唯一一个标签，历史别名（`BTN` / `Edit` / `TreeView` / `Progress` 等）已全部删除。
解析入口统一去前缀取本意名，`<Button>` 这类旧写法不再注册。

## 六、本轮已修 / 剩余缺口

**本轮已修（v0.10.17）**

1. `TextArea` 属性重复：`FontSize`、`Foreground` 不再由控件自声明，统一走基类外观字段，由 `onAppearanceChanged` 转成 `CHARFORMAT`（`XamlRichText.cpp`），schema 已无重名条目。
2. `MonthGrid` 补上 `Value`（`YYYY-MM-DD`，读写）：宿主新增 `getCalendarDate` / `setCalendarDate`（`MCM_GETCURSEL` / `MCM_SETCURSEL`）。
3. `Flickable` 补上 `OffsetX` / `OffsetY`（读写，落到宿主 `setFlickOffset`），不再只能靠手势改视口。
4. 事件契约：`combobox` / `choice` 补 `keydown` / `keyup`（宿主本来就把 `WM_KEYDOWN` 统一派发成 `keydown`，缺的只是契约条目）。

**剩余缺口（按实测，不含猜测）**

1. **只读属性共 7 个**：`TableView.RowCount`、`Menu.SelectedIndex`、`VirtualList.ItemCount`、`GridView.ItemCount`、`Page.PageCount`、`StackView.PageCount`、`SwipeView.PageCount`。只有 `Menu.SelectedIndex` 是设计上的只读回读槽（`XamlMenu.cpp` 里 `Set` 为空，只反射当前选中）；若需要程序化选中菜单项，得补 `setMenuSelected` 宿主接口 + 属性 `Set`。其余 6 个是计数类，只读合理。
2. **`Page` / `StackView` / `SwipeView` 是同一实现**（`XamlPageView`，skin #17），三者属性都是 `SelectedIndex` + `PageCount`，差别仅在语义命名；后续若要真分页/滑动行为，需要在 `XamlPageView` 上分模式，而不是继续加别名。
3. **仍有 5 个控件自有属性为 0**：`Pane`、`SplitView`、`Divider`、`BusyIndicator`、`Flickable`（Flickable 已补偏移，其余 4 个仍只有布局/外观）；`BusyIndicator` 要补 `Running` / `Interval` 得先给 skinbox 加状态接口。
4. **B 档 30 个控件多数只有一个主语义字段**，缺的是"第二层"能力（如 `Button.Icon` / `ListBox.MultiSelect` 之外的 `ItemHeight`、`TreeCtrl.Indent` / `NodeState`），要按实际使用场景逐个补，不预造属性。
