# MFCXaml 控件类名 / 中文名 / QML / WPF / WinForms 对照表

> 由 `tools\gen_control_names.py` 生成，勿手改。
> XAML 标签（类名）就是 `guiload` / `xamlload` 里写的标签；中文名只用于界面与文档。
> QML 列统一按 Qt Quick Controls 2（Qt 5.15 / Qt 6 通用）对照；纯图元（Rectangle/Ellipse/Line/Arc）按 QtQuick 基础类型，窗口类按 QtQuick.Window。
> 档位口径见 `docs\mfcxaml-控件接口清单.md`：S 完整 / A 基本 / B 薄 / C 仅布局。
> QML / WPF / WinForms 三列是"同一套界面换引擎时落在哪个类型"，"QML 方式"只说 QML 侧的同名/组合/自绘；
> 写"无"的表示该平台没有等价内置控件，只能组合或自绘，迁移时是重写而不是换标签。

## 容器（13）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XBorder` | 边框容器 | Rectangle（border.*） | Border | Panel（BorderStyle） | 组合 | — | — |
| `XDivider` | 分隔线 | Rectangle（height: 1）/ HorizontalDivider | Separator / Rectangle | Label（BorderStyle）/ Panel | 组合 | C 仅布局 | 19 |
| `XFlickable` | 拖拽视口 | Flickable | ScrollViewer（PanningMode） | Panel（AutoScroll） | 同名 | B 薄 | 21 |
| `XFrame` | 框架容器 | Frame | Frame / GroupBox | Panel（BorderStyle） | 同名 | B 薄 | 20 |
| `XGridLayout` | 网格控件 | GridLayout / TableView | DataGrid / Grid | TableLayoutPanel | 组合 | B 薄 | 21 |
| `XGroupBox` | 分组框 | GroupBox | GroupBox | GroupBox | 同名 | B 薄 | 20 |
| `XListCtrl` | 列表控件 | ListView / TableView（单列） | ListView | ListView | 组合 | B 薄 | 21 |
| `XPane` | 面板容器 | Item / Pane | Pane | Pane | 组合 | C 仅布局 | 19 |
| `XScrollView` | 滚动窗口 | ScrollView | ScrollViewer | Panel（AutoScroll） | 组合 | B 薄 | 21 |
| `XSplitView` | 分隔条 | SplitView | GridSplitter | SplitContainer | 组合 | C 仅布局 | 19 |
| `XTabBar` | 选项卡 | TabBar + StackLayout | TabBar | TabBar | 组合 | B 薄 | 20 |
| `XTabButton` | 页签按钮 | TabButton | TabItem | TabPage | 同名 | B 薄 | 21 |
| `XTreeView` | 树形控件 | TreeView | TreeView | TreeView | 组合 | B 薄 | 20 |

## 按钮（8）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XButton` | 按钮 | Button | Button | Button | 同名 | B 薄 | 20 |
| `XCheckBox` | 复选框 | CheckBox | CheckBox | CheckBox | 同名 | B 薄 | 21 |
| `XDelayButton` | 长按按钮 | DelayButton | 无（Button + DispatcherTimer） | 无 | 同名 | A 基本 | 22 |
| `XIconButton` | 图标按钮 | Button + icon.source | Button + Image | Button（Image） | 组合 | B 薄 | 21 |
| `XRadioButton` | 单选按钮 | RadioButton | RadioButton | RadioButton | 同名 | B 薄 | 21 |
| `XRoundButton` | 圆形按钮 | RoundButton | Button（圆角模板） | Button（Region 圆形） | 同名 | B 薄 | 21 |
| `XToggleButton` | 开关按钮 | Button + checkable | ToggleButton | CheckBox（Appearance=Button） | 组合 | B 薄 | 21 |
| `XToolButton` | 工具按钮 | ToolButton | Button（ToolBar 内） | ToolStripButton | 同名 | A 基本 | 22 |

## 输入控件（13）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XChoice` | 选择框 | ComboBox（editable:false） | ComboBox（IsEditable=False） | ComboBox（DropDownList） | 同名 | B 薄 | 20 |
| `XCodeEditor` | 代码编辑器 | TextArea（等宽字体 + 行号槽） | TextBox（等宽 + 行号自绘） | TextBox（自绘行号）/ RichTextBox | 组合 | A 基本 | 22 |
| `XComboBox` | 下拉框 | ComboBox | ComboBox | ComboBox | 同名 | A 基本 | 23 |
| `XDatePicker` | 日期选择框 | TextField + MonthGrid | DatePicker | DateTimePicker | 组合 | B 薄 | 20 |
| `XDial` | 旋钮 | Dial | 无（自绘） | 无（自绘） | 同名 | A 基本 | 22 |
| `XListBox` | 列表框 | ListView | ListBox | ListBox | 组合 | A 基本 | 22 |
| `XRangeSlider` | 区间滑块 | RangeSlider | Slider（IsSelectionRangeEnabled） | 无（自绘） | 同名 | A 基本 | 23 |
| `XScrollBar` | 滚动条 | ScrollBar / ScrollView | ScrollBar / ScrollViewer | HScrollBar / VScrollBar | 同名 | S 完整 | 24 |
| `XSlider` | 滑块 | Slider | Slider | TrackBar | 同名 | A 基本 | 22 |
| `XSpinBox` | 数值步进框 | SpinBox | 无内置（TextBox + RepeatButton） | NumericUpDown | 同名 | A 基本 | 22 |
| `XSwitch` | 开关 | Switch | ToggleButton（自定义模板） | CheckBox（自绘开关） | 同名 | B 薄 | 20 |
| `XTextField` | 文本框 | TextField / TextArea | TextField | TextField | 同名 | S 完整 | 24 |
| `XTumbler` | 滚轮选择器 | Tumbler | 无（WinUI 有，WPF 需自绘） | 无（自绘） | 同名 | B 薄 | 21 |

## 显示控件（8）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XBusyIndicator` | 忙碌指示器 | BusyIndicator | ProgressBar（IsIndeterminate） | ProgressBar（Marquee） | 同名 | C 仅布局 | 19 |
| `XHyperlink` | 超链接 | Label + MouseArea（link 样式） | Hyperlink（TextBlock 内联） | LinkLabel | 组合 | B 薄 | 21 |
| `XImage` | 图片 | Image | Image | PictureBox | 同名 | B 薄 | 20 |
| `XLabel` | 文本标签 | Label / Text | Label | Label | 同名 | B 薄 | 20 |
| `XMonthGrid` | 日历 | MonthGrid + DayOfWeekRow | Calendar | MonthCalendar | 组合 | B 薄 | 20 |
| `XProgressBar` | 进度条 | ProgressBar | ProgressBar | ProgressBar | 同名 | A 基本 | 22 |
| `XScrollIndicator` | 滚动指示器 | ScrollIndicator | ScrollBar | 无（Panel 自绘） | 同名 | A 基本 | 22 |
| `XTextArea` | 富文本 | TextArea（textFormat: RichText） | RichTextBox | RichTextBox | 组合 | A 基本 | 22 |

## 数据视图（9）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XGridView` | 网格视图 | GridView | ListView（GridView 视图） | ListView（LargeIcon） | 同名 | S 完整 | 24 |
| `XPage` | 页面 | Page | Frame / TabItem | TabPage / Panel | 同名 | B 薄 | 21 |
| `XPageIndicator` | 页指示器 | PageIndicator | 无（ItemsControl + Ellipse） | 无（自绘） | 同名 | B 薄 | 21 |
| `XPathView` | 路径视图 | PathView | 无（Canvas + Path 自绘） | 无（自绘） | 同名 | B 薄 | 21 |
| `XStackView` | 栈式视图 | StackView | 无（ContentControl 切换） | 无（Panel 切换） | 同名 | B 薄 | 21 |
| `XSwipeView` | 滑动视图 | SwipeView | 无（ScrollViewer 分页自绘） | 无（自绘） | 同名 | B 薄 | 21 |
| `XTableView` | 数据表格 | TableView（model + delegate） | DataGrid | DataGridView | 组合 | A 基本 | 23 |
| `XTreeTable` | 树形表格 | TreeView（6.x）+ 多列 delegate | TreeView + GridView 列 | 无（TreeView + DataGridView 组合） | 组合 | B 薄 | 21 |
| `XVirtualList` | 虚拟列表 | ListView（cacheBuffer 虚拟化） | ListView（VirtualizingStackPanel） | ListView（VirtualMode） | 组合 | A 基本 | 23 |

## 功能条（3）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XMenuBar` | 菜单栏 | MenuBar | Menu | MenuBar | 组合 | B 薄 | 21 |
| `XToolBar` | 工具栏 | ToolBar | ToolBar | ToolBar | 组合 | B 薄 | 21 |
| `XToolSeparator` | 工具分隔符 | ToolSeparator | Separator | ToolStripSeparator | 同名 | B 薄 | 20 |

## 浮层（7）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XDialog` | 对话框 | Dialog | Window（ShowDialog） | Form（ShowDialog） | 同名 | A 基本 | 22 |
| `XDrawer` | 抽屉面板 | Drawer | 无（Popup + 动画） | 无（Panel + 动画） | 同名 | — | — |
| `XMenu` | 弹出菜单 | Menu | ContextMenu | ContextMenuStrip | 同名 | B 薄 | 21 |
| `XMenuItem` | 菜单项 | MenuItem | MenuItem | ToolStripMenuItem | 同名 | A 基本 | 22 |
| `XMenuSeparator` | 菜单分隔符 | MenuSeparator | Separator | ToolStripSeparator | 同名 | B 薄 | 20 |
| `XPopup` | 浮层 | Popup | Popup | ToolStripDropDown | 同名 | A 基本 | 22 |
| `XToolTip` | 提示气泡 | ToolTip | ToolTip | ToolTip | 同名 | B 薄 | 20 |

## 画布（2）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XPowerCanvas` | 配电规划画布 | Canvas / Shape | Canvas（自绘 Path） | Panel（Graphics 自绘） | 自绘 | B 薄 | 20 |
| `XQCanvas` | 通用画布 | Canvas / Item | Canvas（自绘） | Panel（Graphics 自绘） | 自绘 | B 薄 | 21 |

## 绘制图元（4）

| 类名（XAML 标签） | 中文名 | QML（Qt Quick Controls 2） | WPF | WinForms | QML 方式 | 档位 | 属性数 |
|---|---|---|---|---|---|---|---|
| `XArc` | 圆弧 | Shape + PathAngleArc | Path（ArcSegment） | Graphics.DrawArc（自绘） | 自绘 | — | — |
| `XEllipse` | 椭圆 | Shape（PathArc） | Ellipse | Graphics.DrawEllipse（自绘） | 自绘 | — | — |
| `XLine` | 直线 | Shape + Path | Line / Path | Graphics.DrawLine（自绘） | 自绘 | — | — |
| `XRectangle` | 矩形 | Rectangle | Rectangle | Graphics.DrawRectangle（自绘） | 同名 | — | — |

## 属性对应

| MFCXaml 属性 | QML 写法 |
|---|---|
| BackColor / FrontColor | background: Rectangle { color: ... }（QML 无全局底色属性） |
| Foreground / FrontColor(文字) | color（Text / Label）或 palette.text |
| BorderColor / BorderStyle / BorderThickness | Rectangle 的 border.color / border.width；Sunken、Raised 需自绘 |
| Radius | radius |
| Opacity | opacity（0-255 折算成 0.0-1.0） |
| TextAlign | horizontalAlignment / verticalAlignment |
| TextWrapping | wrapMode |
| FontSize / FontFamily / FontWeight / FontStyle | font.pixelSize / font.family / font.weight / font.italic |
| Level | z（叠放顺序，语义等价） |
| TabIndex | 无对应：Qt 用 activeFocusOnTab + KeyNavigation |
| Theme | 无对应：Qt 用 qtquickcontrols2.conf / 自定义 Style |

## 事件对应

| XAML 回调键 | QML 信号处理器 |
|---|---|
| `click` | `onClicked（Button）/ MouseArea.onClicked` |
| `dblclick` | `MouseArea.onDoubleClicked` |
| `change` | `onTextEdited / onTextChanged / onAccepted` |
| `select` | `onCurrentIndexChanged / onCurrentItemChanged` |
| `value` | `onValueChanged / onPositionChanged` |
| `checked` | `onCheckedChanged / onToggled` |
| `dropdown` | `onPressedChanged（Popup 开合）` |
| `scroll` | `WheelHandler.onWheel` |
| `mousedown / mouseup / mousemove` | `MouseArea.onPressed / onReleased / onPositionChanged` |
| `keydown / keyup` | `Keys.onPressed / Keys.onReleased` |
| `gotfocus / lostfocus` | `onActiveFocusChanged` |

## 合计与对应情况

共 **67** 个标签：51 个主控件 + 5 个绘制图元（Line / Rectangle / Ellipse / Arc / Border）。

- **同名**：40 个
- **组合**：22 个
- **自绘**：5 个

同名的那批（Button / CheckBox / ComboBox / Slider / Switch / SpinBox / Dial / Tumbler / StackView / SwipeView / GridView / PathView / Page / Flickable / ToolTip / Menu / Popup 等）换到 QML 只要改标签名和属性写法，布尔与数值语义一致，可以直接对搬。

三处必须注意的结构差异：

1. **表格与列表是 model + delegate**：MFCXaml 的 `Rows` / `Items` / `Columns` 是字符串直填，QML 侧要换 `ListModel` 或 `QAbstractTableModel` + delegate，这是迁移时工作量最大的地方。
2. **没有原生 TreeTable / GridCtrl**：QML 需 TreeView 或多列 delegate 自己组，不是换个标签就能完成。
3. **自绘控件（PowerCanvas / QCanvas / Line / Ellipse / Arc）**：MFCXaml 走 GDI 自绘，QML 侧对应 Canvas 或 Shape，绘制代码要重写而不是映射。

另外两条工程差异：QML 默认按 96 DPI 的像素单位布局，MFC 是物理像素，尺寸要按 `devicePixelRatio` 折算；QML 的焦点与 Tab 顺序靠 `activeFocusOnTab` + `KeyNavigation`，不像 XAML 有 `TabIndex`。

不在本表的旧别名（已淘汰，勿再使用）：`TrackBar`（Slider 的旧名）、`Label`（TextBlock）、`Edit`（TextBox）、`Progress`（ProgressBar）、`TreeView`（TreeCtrl）、`ListView`（ListCtrl）。


## QML 写法对照（属性名）

XAML 里可以直接按 QML 的属性名写，渲染仍是 MFC 自绘；旧的 WinForms 写法继续可用。

| QML 写法 | 等价旧写法 |
|---|---|
| `text` | `Text` |
| `color` | `Foreground` |
| `background` | `BackColor` |
| `foreground` | `FrontColor` |
| `radius` | `Radius` |
| `opacity` | `Opacity` |
| `z` | `Level` |
| `border.color` | `BorderColor` |
| `border.width` | `BorderThickness` |
| `border.style` | `BorderStyle` |
| `font.pixelSize` | `FontSize` |
| `font.family` | `FontFamily` |
| `font.weight` | `FontWeight` |
| `font.style` | `FontStyle` |
| `horizontalAlignment` | `TextAlign` |
| `verticalAlignment` | `VerticalAlignment` |
| `wrapMode` | `TextWrapping` |
| `padding` | `Padding` |
| `checked` | `Checked` |
| `checkable` | `Checkable` |
| `value` | `Value` |
| `from` | `Minimum` |
| `to` | `Maximum` |
| `currentIndex` | `SelectedIndex` |
| `model` | `Items` |
| `readOnly` | `IsReadOnly` |
| `placeholderText` | `Placeholder` |
| `maximumLength` | `MaxLength` |
| `icon.source` | `IconSource` |
| `icon.name` | `IconSource` |
| `display` | `Display` |
| `modal` | `Modal` |
| `orientation` | `Orientation` |
| `delay` | `Delay` |
| `position` | `Position` |
| `size` | `Size` |
| `columns` | `Columns` |
| `rows` | `Rows` |
| `count` | `Count` |
| `title` | `Title` |
| `source` | `Image` |
| `link` | `Url` |
| `selectionMode` | `SelectionMode` |
| `header` | `Header` |
| `rowCount` | `RowCount` |
| `pageCount` | `PageCount` |
| `contentX` | `OffsetX` |
| `contentY` | `OffsetY` |
| `firstValue` | `LowValue` |
| `secondValue` | `HighValue` |
| `itemHeight` | `ItemHeight` |
| `cellWidth` | `CellWidth` |
| `cellHeight` | `CellHeight` |

## QML 写法对照（事件名）

事件名同样接受 QML 的信号处理器写法（`onClicked="on_run"`）。

| QML 写法 | 等价旧写法 | 说明 |
|---|---|---|
| `onClicked` | `Click` | 单击 |
| `onDoubleClicked` | `DoubleClick` | 双击 |
| `onTextChanged` | `TextChanged` | 文本变化 |
| `onCheckedChanged` | `CheckedChanged` | 勾选变化 |
| `onValueChanged` | `ValueChanged` | 取值变化 |
| `onSelectionChanged` | `SelectionChanged` | 选中项变化 |
| `onPressed` / `onReleased` | `MouseDown` / `MouseUp` | 按下 / 松开 |
| `onPositionChanged` | `MouseMove` | 移动 |
| `onActivated` / `onDeactivated` | `GotFocus` / `LostFocus` | 获得 / 失去焦点 |
| `onWheel` | `Scroll` | 滚轮 |

## QML 写法对照（布局）

Grid 里的位置和尺寸可以直接按 QML 的附加属性写，落到的是 Grid 行/列与 Width/Height。

| QML 写法 | 等价旧写法 | 说明 |
|---|---|---|
| `Layout.row` | `Grid.Row` | 所在行 |
| `Layout.column` | `Grid.Column` | 所在列 |
| `Layout.rowSpan` | `Grid.RowSpan` | 跨行 |
| `Layout.columnSpan` | `Grid.ColumnSpan` | 跨列 |
| `Layout.preferredWidth` | `Width` | 声明尺寸（未写 Width 时生效） |
| `Layout.preferredHeight` | `Height` | 声明尺寸（未写 Height 时生效） |
| `Layout.margins` | `Margin` | 外边距 |
| `x` / `y` | `Left` / `Top` | 绝对定位 |
| `width` / `height` | `Width` / `Height` | 尺寸 |

锚定写法（`anchors.*` 锚到父容器内框，v1 只支持 parent）：

| QML 写法 | 效果 |
|---|---|
| `anchors.fill="parent"` | 占满父容器内框 |
| `anchors.left` / `right` / `top` / `bottom` | 贴左/右/上/下 |
| `anchors.centerIn="parent"` | 水平垂直居中 |
| `anchors.horizontalCenter` / `verticalCenter` | 单轴居中 |
| `anchors.margins` | 四边内缩 |

`Layout.fillWidth` / `Layout.fillHeight` 仍只做写法兼容，实际拉伸由容器（StackPanel / Grid / Pane）决定。