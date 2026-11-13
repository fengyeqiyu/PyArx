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
| 控件（注册主标签） | 51 |
| 属性条目合计（含通用） | 1076 |
| 其中通用属性 | 969（51 控件 × 19） |
| 其中自有属性 | 107 |
| 只读属性 | 7 |
| 事件键（内部名） | 15（click / dblclick / change / select / value / checked / dropdown / scroll / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus） |

档位分布：S 完整 **3** 个 · A 基本 **12** 个 · B 薄 **32** 个 · C 仅布局 **4** 个 · 

## 三、逐控件清单

| 控件 | 实现 | 自有属性（读写） | 只读 | 事件 | 档位 | 源文件 |
|---|---|---|---|---|---|---|
| **Button** | 原生 MFC Button | Text | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlButton.cpp` |
| **TextBlock** | 原生 MFC Static | Text | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlTextBlock.cpp` |
| **TextBox** | 原生 MFC Edit | Text / IsReadOnly / Multiline / MaxLength / Placeholder | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | S 完整 | `XamlTextBox.cpp` |
| **ListBox** | 原生 MFC ListBox | Items / SelectedIndex / SelectionMode | — | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlListBox.cpp` |
| **CodeEditor** | C++ 自绘编辑器 | Text / Syntax / IsReadOnly | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | A 基本 | `XamlCodeEditor.cpp` |
| **CheckBox** | 原生 MFC Button(BS_AUTOCHECKBOX) | Text / Checked | — | click / checked / dblclick / gotfocus / lostfocus | B 薄 | `XamlCheckBox.cpp` |
| **RadioButton** | 原生 MFC Button(BS_AUTORADIO) | Text / Checked | — | click / checked / gotfocus / lostfocus | B 薄 | `XamlRadioButton.cpp` |
| **GroupBox** | 原生 MFC Button(BS_GROUPBOX) | Header | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlGroupBox.cpp` |
| **ComboBox** | 原生 MFC ComboBox | Items / Text / SelectedIndex / DropDownStyle | — | select / change / dropdown / dblclick / gotfocus / lostfocus | A 基本 | `XamlComboBox.cpp` |
| **ProgressBar** | 原生 MFC msctls_progress32 | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlProgressBar.cpp` |
| **Slider** | 原生 MFC msctls_trackbar32 | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlSlider.cpp` |
| **TabControl** | 原生 MFC SysTabControl32 | Pages | — | click / select / gotfocus / lostfocus | B 薄 | `XamlTabControl.cpp` |
| **TreeCtrl** | 原生 MFC SysTreeView32 | Nodes | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlTreeCtrl.cpp` |
| **ListCtrl** | 原生 MFC SysListView32 | Columns / Rows | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlListCtrl.cpp` |
| **Choice** | 原生 MFC ComboBox(CBS_DROPDOWNLIST) | Items | — | select / dropdown / dblclick / gotfocus / lostfocus | B 薄 | `XamlChoice.cpp` |
| **DatePicker** | 原生 MFC SysDateTimePick32 | Value | — | select / value / dblclick / gotfocus / lostfocus | B 薄 | `XamlDatePicker.cpp` |
| **Panel** | 原生 MFC Static 容器 | — | — | click / dblclick / mousedown / mouseup / mousemove | C 仅布局 | `XamlPanel.cpp` |
| **Splitter** | 原生 MFC 自注册拖动条 | — | — | mousedown / mouseup / mousemove / dblclick | C 仅布局 | `XamlSplitter.cpp` |
| **StaticLine** | 原生 MFC Static(SS_ETCHEDHORZ) | — | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | C 仅布局 | `XamlStaticLine.cpp` |
| **PowerCanvas** | 宿主自绘画布（配电规划图） | Data | — | click / dblclick / mousedown / mouseup / mousemove / scroll / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlPowerCanvas.cpp` |
| **Hyperlink** | 原生 MFC SysLink | Text / Url | — | click / dblclick / gotfocus / lostfocus | B 薄 | `XamlHyperlink.cpp` |
| **RichText** | 原生 MFC RICHEDIT50W | Text / Bold / Italic | — | change / keydown / keyup / gotfocus / lostfocus / click / dblclick | A 基本 | `XamlRichText.cpp` |
| **ToggleButton** | 原生 MFC Button(BS_AUTOCHECKBOX 自绘) | Text / Checked | — | click / checked / dblclick / gotfocus / lostfocus | B 薄 | `XamlToggleButton.cpp` |
| **BitmapButton** | 原生 MFC Button(BS_BITMAP) | Text / Image | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlBitmapButton.cpp` |
| **StaticBitmap** | 原生 MFC Static(SS_BITMAP) | Image | — | click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlStaticBitmap.cpp` |
| **GridCtrl** | 原生 MFC 自注册网格 | Columns / Rows | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlGrid.cpp` |
| **ScrolledWindow** | 原生 MFC 自注册滚动容器 | ScrollMin / ScrollMax | — | scroll / click / gotfocus / lostfocus | B 薄 | `XamlScrolledWindow.cpp` |
| **CalendarCtrl** | 原生 MFC SysMonthCal32 | Value | — | select / value / dblclick / gotfocus / lostfocus | B 薄 | `XamlCalendarCtrl.cpp` |
| **Switch** | 自绘 skin #1 Switch | Checked | — | click / checked / value / gotfocus / lostfocus | B 薄 | `XamlSwitch.cpp` |
| **SpinBox** | 自绘 skin #2 Spin | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlSpinBox.cpp` |
| **RangeSlider** | 自绘 skin #3 Range | LowValue / HighValue / Minimum / Maximum | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlRangeSlider.cpp` |
| **BusyIndicator** | 自绘 skin #4 Busy | — | — | click / gotfocus / lostfocus | C 仅布局 | `XamlBusyIndicator.cpp` |
| **PageIndicator** | 自绘 skin #5 Pages | Count / SelectedIndex | — | click / select / gotfocus / lostfocus | B 薄 | `XamlPageIndicator.cpp` |
| **Dial** | 自绘 skin #6 Dial | Minimum / Maximum / Value | — | value / scroll / gotfocus / lostfocus / click | A 基本 | `XamlDial.cpp` |
| **Tumbler** | 自绘 skin #7 Tumbler | Count / SelectedIndex | — | value / scroll / gotfocus / lostfocus / click | B 薄 | `XamlTumbler.cpp` |
| **ScrollBar** | 自绘 skin #8 ScrollBar | Minimum / Maximum / Value / PageSize / Orientation | — | value / scroll / gotfocus / lostfocus | S 完整 | `XamlScrollBar.cpp` |
| **TableView** | 自绘 skin #9 Table（独立控件套装） | Columns / Rows / SelectedIndex / RowCount | RowCount | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlTableView.cpp` |
| **TreeTable** | 自绘 skin #10 Tree | Nodes / SelectedIndex | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlTreeView.cpp` |
| **ToolStrip** | 自绘 skin #11 ToolBar | Items / SelectedIndex | — | click / select / dropdown | B 薄 | `XamlToolBar.cpp` |
| **ToolTip** | 自绘 skin #12 ToolTip | Text | — | click | B 薄 | `XamlToolTip.cpp` |
| **Menu** | 自绘 skin #13 Menu | Items / SelectedIndex | SelectedIndex | click / select | B 薄 | `XamlMenu.cpp` |
| **MenuStrip** | 自绘 skin #14 MenuBar | Items / SelectedIndex | — | click / select | B 薄 | `XamlMenuBar.cpp` |
| **Popup** | 自绘 skin #15 Popup | Title / Content / Lines | — | click / gotfocus / lostfocus | A 基本 | `XamlPopup.cpp` |
| **VirtualList** | 自绘 skin #16 ListView（虚拟化） | Items / ItemHeight / SelectedIndex / ItemCount | ItemCount | select / click / dblclick / scroll / gotfocus / lostfocus | A 基本 | `XamlListView.cpp` |
| **StackView** | 自绘 skin #17 Pager（XamlPageView 子类） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlStackView.h` |
| **SwipeView** | 自绘 skin #17 Pager（XamlPageView 子类） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlSwipeView.h` |
| **GridView** | 自绘 skin #18 GridView | Items / SelectedIndex / CellWidth / CellHeight / ItemCount | ItemCount | select / click / dblclick / scroll / gotfocus / lostfocus | S 完整 | `XamlGridView.cpp` |
| **PathView** | 自绘 skin #19 PathView | Items / SelectedIndex | — | select / click / dblclick / scroll / gotfocus / lostfocus | B 薄 | `XamlPathView.cpp` |
| **QCanvas** | 自绘 skin #20 QCanvas（命令脚本画布） | Commands / Script | — | click / dblclick / mousedown / mouseup / mousemove / scroll / keydown / keyup / gotfocus / lostfocus | B 薄 | `XamlQCanvas.cpp` |
| **Page** | 自绘 skin #17 Pager（XamlPageView） | SelectedIndex / PageCount | PageCount | click / select / gotfocus / lostfocus | B 薄 | `XamlPage.cpp` |
| **Flickable** | 自绘 skin #21 Flick | OffsetX / OffsetY | — | scroll / click / mousedown / mouseup / mousemove | B 薄 | `XamlFlickable.cpp` |

## 四、分档结论

**S 完整（自有 ≥5，可直接当业务控件用）**：TextBox(5)、ScrollBar(5)、GridView(5)

**A 基本（3–4，够用但还缺边角能力）**：ListBox(3)、CodeEditor(3)、ComboBox(4)、ProgressBar(3)、Slider(3)、RichText(3)、SpinBox(3)、RangeSlider(4)、Dial(3)、TableView(4)、Popup(3)、VirtualList(4)

**B 薄（1–2，只有主语义那一个字段）**：Button(1)、TextBlock(1)、CheckBox(2)、RadioButton(2)、GroupBox(1)、TabControl(1)、TreeCtrl(1)、ListCtrl(2)、Choice(1)、DatePicker(1)、PowerCanvas(1)、Hyperlink(2)、ToggleButton(2)、BitmapButton(2)、StaticBitmap(1)、GridCtrl(2)、ScrolledWindow(2)、CalendarCtrl(1)、Switch(1)、PageIndicator(2)、Tumbler(2)、TreeTable(2)、ToolStrip(2)、ToolTip(1)、Menu(2)、MenuStrip(2)、StackView(2)、SwipeView(2)、PathView(2)、QCanvas(2)、Page(2)、Flickable(2)

**C 仅布局（自有 0，属性面上等同一块带样式的面板）**：Panel、Splitter、StaticLine、BusyIndicator

C 档不是"没实现"——这些控件在 C++ 里都有真实原生窗口/自绘实现、事件也照常派发，只是**业务字段没有暴露成属性**。要让它们进设计器可配置，得按需要补属性表条目；这也是 `plugins` 层用 XAML 组合补位的典型对象（组合态用子控件属性，不硬造新属性）。

## 五、别名现状（建议只用主标签）

同一实现注册了多个标签，`XAML` 里写别名能跑，但会让人分不清"这到底是哪个控件"。按项目规矩**只用第一列主标签**，别名仅作历史兼容：

| 主标签 | 历史别名（勿用） |
|---|---|
| Button | button, BTN |
| TextBlock | textblock, Label, label |
| TextBox | textbox, Edit, edit |
| ListBox | listbox |
| CodeEditor | codeeditor, CodeView, codeview, XmlEditor |
| CheckBox | checkbox, Checkbox |
| RadioButton | radiobutton, Radio, radio |
| GroupBox | groupbox, Group, group |
| ComboBox | combobox, Combo, combo |
| ProgressBar | progressbar, Progress, progress |
| Slider | slider |
| TabControl | tabcontrol, TabWidget, tabwidget |
| TreeCtrl | treectrl, TreeView, tree |
| ListCtrl | listctrl, ListView, listctrl_view |
| Choice | choice |
| DatePicker | datepicker, DatePickerCtrl |
| Panel | panel |
| Splitter | splitter, SplitterWindow |
| StaticLine | staticline |
| PowerCanvas | powercanvas, PlanCanvas |
| Hyperlink | hyperlink, HyperlinkCtrl |
| RichText | richtext, RichTextCtrl |
| ToggleButton | togglebutton, ToggleBtn |
| BitmapButton | bitmapbutton, BitmapBtn |
| StaticBitmap | staticbitmap |
| GridCtrl | gridctrl, DataGrid |
| ScrolledWindow | scrolledwindow |
| CalendarCtrl | calendar, Calendar |
| Switch | switch, ToggleSwitch |
| SpinBox | spinbox, NumberBox, NumericUpDown |
| RangeSlider | rangeslider, SliderRange |
| BusyIndicator | busyindicator, Spinner, ActivityIndicator |
| PageIndicator | pageindicator, PagerDots |
| Dial | dial, Knob, Gauge |
| Tumbler | tumbler, Wheel |
| ScrollBar | scrollbar, VScrollBar, HScrollBar |
| TableView | tableview, DataTable, GridTable |
| TreeTable | treetable, HierarchicalTable |
| ToolStrip | toolstrip, ToolButtonBar |
| ToolTip | tooltip, Hint |
| Menu | menu, ContextMenu, PopupMenu |
| MenuStrip | menustrip, MenuBarControl, TopMenuBar |
| Popup | popup, Overlay（Dialog / drawer 同实现） |
| VirtualList | virtuallist, ListViewV, ModelList |
| StackView | stackview, PageView, CarouselView |
| SwipeView | swipeview, PageSwipe |
| GridView | gridview, TileView |
| PathView | pathview, CoverFlow |
| QCanvas | qcanvas, DrawCanvas, PaintCanvas, CanvasSurface, Painter |
| Page | PagedView, WizardPage |
| Flickable | flickable, FlickArea |

冲突点（别名被别的层占着，所以自绘控件另起主名）：

- `TreeView` / `tree` 归原生 `SysTreeView32`（`TreeCtrl`），自绘层次表只能用 `TreeTable`。
- `ToolBar` / `toolbar` 归布局容器，自绘按钮条只能用 `ToolStrip`。
- `ListView` 归原生 `SysListView32`（`ListCtrl`），虚拟列表只能用 `VirtualList`。
- `Canvas` 归绝对布局容器，画命令画布只能用 `QCanvas`（正名 `DrawCanvas` / `PaintCanvas`）。
- `Dialog` / `Drawer` 与 `Popup` 同一实现（标题条 + 正文），属性表相同。

## 六、本轮已修 / 剩余缺口

**本轮已修（v0.10.17）**

1. `RichText` 属性重复：`FontSize`、`Foreground` 不再由控件自声明，统一走基类外观字段，由 `onAppearanceChanged` 转成 `CHARFORMAT`（`XamlRichText.cpp`），schema 已无重名条目。
2. `CalendarCtrl` 补上 `Value`（`YYYY-MM-DD`，读写）：宿主新增 `getCalendarDate` / `setCalendarDate`（`MCM_GETCURSEL` / `MCM_SETCURSEL`）。
3. `Flickable` 补上 `OffsetX` / `OffsetY`（读写，落到宿主 `setFlickOffset`），不再只能靠手势改视口。
4. 事件契约：`combobox` / `choice` 补 `keydown` / `keyup`（宿主本来就把 `WM_KEYDOWN` 统一派发成 `keydown`，缺的只是契约条目）。

**剩余缺口（按实测，不含猜测）**

1. **只读属性共 7 个**：`TableView.RowCount`、`Menu.SelectedIndex`、`VirtualList.ItemCount`、`GridView.ItemCount`、`Page.PageCount`、`StackView.PageCount`、`SwipeView.PageCount`。只有 `Menu.SelectedIndex` 是设计上的只读回读槽（`XamlMenu.cpp` 里 `Set` 为空，只反射当前选中）；若需要程序化选中菜单项，得补 `setMenuSelected` 宿主接口 + 属性 `Set`。其余 6 个是计数类，只读合理。
2. **`Page` / `StackView` / `SwipeView` 是同一实现**（`XamlPageView`，skin #17），三者属性都是 `SelectedIndex` + `PageCount`，差别仅在语义命名；后续若要真分页/滑动行为，需要在 `XamlPageView` 上分模式，而不是继续加别名。
3. **仍有 5 个控件自有属性为 0**：`Panel`、`Splitter`、`StaticLine`、`BusyIndicator`、`Flickable`（Flickable 已补偏移，其余 4 个仍只有布局/外观）；`BusyIndicator` 要补 `Running` / `Interval` 得先给 skinbox 加状态接口。
4. **B 档 30 个控件多数只有一个主语义字段**，缺的是"第二层"能力（如 `Button.Icon` / `ListBox.MultiSelect` 之外的 `ItemHeight`、`TreeCtrl.Indent` / `NodeState`），要按实际使用场景逐个补，不预造属性。
