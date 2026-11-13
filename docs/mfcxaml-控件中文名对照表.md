# MFCXaml 控件类名 / 中文名 / QML 对照表

> 由 `tools\gen_control_names.py` 生成，勿手改。
> XAML 标签（类名）就是 `guiload` / `xamlload` 里写的标签；中文名只用于界面与文档。
> 档位口径见 `docs\mfcxaml-控件接口清单.md`：S 完整 / A 基本 / B 薄 / C 仅布局。
> QML 一列是"同一套界面换引擎时落在哪个类型"，同名即语义基本等价。

## 容器（11）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XBorder` | 边框容器 | Rectangle（border.*） | 组合 | — | 160x100 | — | — |
| `XFlickable` | 拖拽视口 | Flickable | 同名 | B 薄 | 320x160 | 21 | XamlFlickable.cpp |
| `XGridCtrl` | 网格控件 | GridLayout / TableView | 组合 | B 薄 | 200x120 | 21 | XamlGrid.cpp |
| `XGroupBox` | 分组框 | GroupBox | 同名 | B 薄 | 160x80 | 20 | XamlGroupBox.cpp |
| `XListCtrl` | 列表控件 | ListView / TableView（单列） | 组合 | B 薄 | 160x120 | 21 | XamlListCtrl.cpp |
| `XPanel` | 面板容器 | Item / Pane | 组合 | C 仅布局 | 160x100 | 19 | XamlPanel.cpp |
| `XScrolledWindow` | 滚动窗口 | ScrollView | 组合 | B 薄 | 200x120 | 21 | XamlScrolledWindow.cpp |
| `XSplitter` | 分隔条 | SplitView | 组合 | C 仅布局 | 8x200 | 19 | XamlSplitter.cpp |
| `XStaticLine` | 分隔线 | Rectangle（height: 1）/ HorizontalDivider | 组合 | C 仅布局 | 120x4 | 19 | XamlStaticLine.cpp |
| `XTabControl` | 选项卡 | TabBar + StackLayout | 组合 | B 薄 | 200x120 | 20 | XamlTabControl.cpp |
| `XTreeCtrl` | 树形控件 | TreeView | 组合 | B 薄 | 160x120 | 20 | XamlTreeCtrl.cpp |

## 按钮（5）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XBitmapButton` | 图标按钮 | Button + icon.source | 组合 | B 薄 | 100x28 | 21 | XamlBitmapButton.cpp |
| `XButton` | 按钮 | Button | 同名 | B 薄 | 100x28 | 20 | XamlButton.cpp |
| `XCheckBox` | 复选框 | CheckBox | 同名 | B 薄 | 100x28 | 21 | XamlCheckBox.cpp |
| `XRadioButton` | 单选按钮 | RadioButton | 同名 | B 薄 | 100x28 | 21 | XamlRadioButton.cpp |
| `XToggleButton` | 开关按钮 | Button + checkable | 组合 | B 薄 | 100x28 | 21 | XamlToggleButton.cpp |

## 输入控件（13）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XChoice` | 选择框 | ComboBox（editable:false） | 同名 | B 薄 | 120x28 | 20 | XamlChoice.cpp |
| `XCodeEditor` | 代码编辑器 | TextArea（等宽字体 + 行号槽） | 组合 | A 基本 | 360x160 | 22 | XamlCodeEditor.cpp |
| `XComboBox` | 下拉框 | ComboBox | 同名 | A 基本 | 120x28 | 23 | XamlComboBox.cpp |
| `XDatePicker` | 日期选择框 | TextField + MonthGrid | 组合 | B 薄 | 140x28 | 20 | XamlDatePicker.cpp |
| `XDial` | 旋钮 | Dial | 同名 | A 基本 | 88x88 | 22 | XamlDial.cpp |
| `XListBox` | 列表框 | ListView | 组合 | A 基本 | 120x100 | 22 | XamlListBox.cpp |
| `XRangeSlider` | 区间滑块 | RangeSlider | 同名 | A 基本 | 200x34 | 23 | XamlRangeSlider.cpp |
| `XScrollBar` | 滚动条 | ScrollBar / ScrollView | 同名 | S 完整 | 200x18 | 24 | XamlScrollBar.cpp |
| `XSlider` | 滑块 | Slider | 同名 | A 基本 | 200x34 | 22 | XamlSlider.cpp |
| `XSpinBox` | 数值步进框 | SpinBox | 同名 | A 基本 | 120x30 | 22 | XamlSpinBox.cpp |
| `XSwitch` | 开关 | Switch | 同名 | B 薄 | 64x28 | 20 | XamlSwitch.cpp |
| `XTextBox` | 文本框 | TextField / TextArea | 同名 | S 完整 | 120x28 | 24 | XamlTextBox.cpp |
| `XTumbler` | 滚轮选择器 | Tumbler | 同名 | B 薄 | 88x96 | 21 | XamlTumbler.cpp |

## 显示控件（7）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XBusyIndicator` | 忙碌指示器 | BusyIndicator | 同名 | C 仅布局 | 56x56 | 19 | XamlBusyIndicator.cpp |
| `XCalendarCtrl` | 日历 | MonthGrid + DayOfWeekRow | 组合 | B 薄 | 180x160 | 20 | XamlCalendarCtrl.cpp |
| `XHyperlink` | 超链接 | Label + MouseArea（link 样式） | 组合 | B 薄 | 120x24 | 21 | XamlHyperlink.cpp |
| `XProgressBar` | 进度条 | ProgressBar | 同名 | A 基本 | 120x16 | 22 | XamlProgressBar.cpp |
| `XRichText` | 富文本 | TextArea（textFormat: RichText） | 组合 | A 基本 | 200x100 | 22 | XamlRichText.cpp |
| `XStaticBitmap` | 图片 | Image | 同名 | B 薄 | 80x80 | 20 | XamlStaticBitmap.cpp |
| `XTextBlock` | 文本标签 | Label / Text | 同名 | B 薄 | 100x24 | 20 | XamlTextBlock.cpp |

## 数据视图（9）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XGridView` | 网格视图 | GridView | 同名 | S 完整 | 320x160 | 24 | XamlGridView.cpp |
| `XPage` | 页面 | Page | 同名 | B 薄 | 360x160 | 21 | XamlPage.cpp |
| `XPageIndicator` | 页指示器 | PageIndicator | 同名 | B 薄 | 160x22 | 21 | XamlPageIndicator.cpp |
| `XPathView` | 路径视图 | PathView | 同名 | B 薄 | 320x140 | 21 | XamlPathView.cpp |
| `XStackView` | 栈式视图 | StackView | 同名 | B 薄 | 360x160 | 21 | XamlStackView.h |
| `XSwipeView` | 滑动视图 | SwipeView | 同名 | B 薄 | 360x160 | 21 | XamlSwipeView.h |
| `XTableView` | 数据表格 | TableView（model + delegate） | 组合 | A 基本 | 160x32 | 23 | XamlTableView.cpp |
| `XTreeTable` | 树形表格 | TreeView（6.x）+ 多列 delegate | 组合 | B 薄 | 320x140 | 21 | XamlTreeView.cpp |
| `XVirtualList` | 虚拟列表 | ListView（cacheBuffer 虚拟化） | 组合 | A 基本 | 240x140 | 23 | XamlListView.cpp |

## 功能条（2）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XMenuStrip` | 菜单栏 | MenuBar | 组合 | B 薄 | 360x28 | 21 | XamlMenuBar.cpp |
| `XToolStrip` | 工具栏 | ToolBar | 组合 | B 薄 | 360x32 | 21 | XamlToolBar.cpp |

## 浮层（3）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XMenu` | 弹出菜单 | Menu | 同名 | B 薄 | 200x120 | 21 | XamlMenu.cpp |
| `XPopup` | 浮层 | Popup | 同名 | A 基本 | 320x160 | 22 | XamlPopup.cpp |
| `XToolTip` | 提示气泡 | ToolTip | 同名 | B 薄 | 160x24 | 20 | XamlToolTip.cpp |

## 画布（2）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XPowerCanvas` | 配电规划画布 | Canvas / Shape | 自绘 | B 薄 | 420x260 | 20 | XamlPowerCanvas.cpp |
| `XQCanvas` | 通用画布 | Canvas / Item | 自绘 | B 薄 | 420x200 | 21 | XamlQCanvas.cpp |

## 绘制图元（4）

| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|---|
| `XArc` | 圆弧 | Shape + PathAngleArc | 自绘 | — | 140x100 | — | — |
| `XEllipse` | 椭圆 | Shape（PathArc） | 自绘 | — | 120x120 | — | — |
| `XLine` | 直线 | Shape + Path | 自绘 | — | 120x2 | — | — |
| `XRectangle` | 矩形 | Rectangle | 同名 | — | 160x90 | — | — |

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

共 **56** 个标签：51 个主控件 + 5 个绘制图元（Line / Rectangle / Ellipse / Arc / Border）。

- **同名**：29 个
- **组合**：22 个
- **自绘**：5 个

同名的那批（Button / CheckBox / ComboBox / Slider / Switch / SpinBox / Dial / Tumbler / StackView / SwipeView / GridView / PathView / Page / Flickable / ToolTip / Menu / Popup 等）换到 QML 只要改标签名和属性写法，布尔与数值语义一致，可以直接对搬。

三处必须注意的结构差异：

1. **表格与列表是 model + delegate**：MFCXaml 的 `Rows` / `Items` / `Columns` 是字符串直填，QML 侧要换 `ListModel` 或 `QAbstractTableModel` + delegate，这是迁移时工作量最大的地方。
2. **没有原生 TreeTable / GridCtrl**：QML 需 TreeView 或多列 delegate 自己组，不是换个标签就能完成。
3. **自绘控件（PowerCanvas / QCanvas / Line / Ellipse / Arc）**：MFCXaml 走 GDI 自绘，QML 侧对应 Canvas 或 Shape，绘制代码要重写而不是映射。

另外两条工程差异：QML 默认按 96 DPI 的像素单位布局，MFC 是物理像素，尺寸要按 `devicePixelRatio` 折算；QML 的焦点与 Tab 顺序靠 `activeFocusOnTab` + `KeyNavigation`，不像 XAML 有 `TabIndex`。

不在本表的旧别名（已淘汰，勿再使用）：`TrackBar`（Slider 的旧名）、`Label`（TextBlock）、`Edit`（TextBox）、`Progress`（ProgressBar）、`TreeView`（TreeCtrl）、`ListView`（ListCtrl）。
