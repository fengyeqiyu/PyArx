# MFCXaml 控件类名 / 中文名对照表

> 由 `tools\gen_control_names.py` 生成，勿手改。
> XAML 标签（类名）就是 `guiload` / `xamlload` 里写的标签；中文名只用于界面与文档。
> 档位口径见 `docs\mfcxaml-控件接口清单.md`：S 完整 / A 基本 / B 薄 / C 仅布局。

## 容器（11）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `Border` | 边框容器 | — | — | 160x100 | — | — |
| `Flickable` | 拖拽视口 | 自绘 skin #21 Flick | B 薄 | 320x160 | 21 | XamlFlickable.cpp |
| `GridCtrl` | 网格控件 | 原生 MFC 自注册网格 | B 薄 | 200x120 | 21 | XamlGrid.cpp |
| `GroupBox` | 分组框 | 原生 MFC Button (BS\_GROUPBOX) | B 薄 | 160x80 | 20 | XamlGroupBox.cpp |
| `ListCtrl` | 列表控件 | 原生 MFC SysListView32 | B 薄 | 160x120 | 21 | XamlListCtrl.cpp |
| `Panel` | 面板容器 | 原生 MFC Static 容器 | C 仅布局 | 160x100 | 19 | XamlPanel.cpp |
| `ScrolledWindow` | 滚动窗口 | 原生 MFC 自注册滚动容器 | B 薄 | 200x120 | 21 | XamlScrolledWindow.cpp |
| `Splitter` | 分隔条 | 原生 MFC 自注册拖动条 | C 仅布局 | 8x200 | 19 | XamlSplitter.cpp |
| `StaticLine` | 分隔线 | 原生 MFC Static (SS\_ETCHEDHORZ) | C 仅布局 | 120x4 | 19 | XamlStaticLine.cpp |
| `TabControl` | 选项卡 | 原生 MFC SysTabControl32 | B 薄 | 200x120 | 20 | XamlTabControl.cpp |
| `TreeCtrl` | 树形控件 | 原生 MFC SysTreeView32 | B 薄 | 160x120 | 20 | XamlTreeCtrl.cpp |

## 按钮（5）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `BitmapButton` | 图标按钮 | 原生 MFC Button (BS\_BITMAP) | B 薄 | 100x28 | 21 | XamlBitmapButton.cpp |
| `Button` | 按钮 | 原生 MFC Button | B 薄 | 100x28 | 20 | XamlButton.cpp |
| `CheckBox` | 复选框 | 原生 MFC Button (BS\_AUTOCHECKBOX) | B 薄 | 100x28 | 21 | XamlCheckBox.cpp |
| `RadioButton` | 单选按钮 | 原生 MFC Button (BS\_AUTORADIO) | B 薄 | 100x28 | 21 | XamlRadioButton.cpp |
| `ToggleButton` | 开关按钮 | 原生 MFC Button (BS\_AUTOCHECKBOX 自绘) | B 薄 | 100x28 | 21 | XamlToggleButton.cpp |

## 输入控件（13）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `Choice` | 选择框 | 原生 MFC ComboBox (CBS\_DROPDOWNLIST) | B 薄 | 120x28 | 20 | XamlChoice.cpp |
| `CodeEditor` | 代码编辑器 | C++ 自绘编辑器 | A 基本 | 360x160 | 22 | XamlCodeEditor.cpp |
| `ComboBox` | 下拉框 | 原生 MFC ComboBox | A 基本 | 120x28 | 23 | XamlComboBox.cpp |
| `DatePicker` | 日期选择框 | 原生 MFC SysDateTimePick32 | B 薄 | 140x28 | 20 | XamlDatePicker.cpp |
| `Dial` | 旋钮 | 自绘 skin #6 Dial | A 基本 | 88x88 | 22 | XamlDial.cpp |
| `ListBox` | 列表框 | 原生 MFC ListBox | A 基本 | 120x100 | 22 | XamlListBox.cpp |
| `RangeSlider` | 区间滑块 | 自绘 skin #3 Range | A 基本 | 200x34 | 23 | XamlRangeSlider.cpp |
| `ScrollBar` | 滚动条 | 自绘 skin #8 ScrollBar | S 完整 | 200x18 | 24 | XamlScrollBar.cpp |
| `Slider` | 滑块 | 原生 MFC msctls\_trackbar32 | A 基本 | 200x34 | 22 | XamlSlider.cpp |
| `SpinBox` | 数值步进框 | 自绘 skin #2 Spin | A 基本 | 120x30 | 22 | XamlSpinBox.cpp |
| `Switch` | 开关 | 自绘 skin #1 Switch | B 薄 | 64x28 | 20 | XamlSwitch.cpp |
| `TextBox` | 文本框 | 原生 MFC Edit | S 完整 | 120x28 | 24 | XamlTextBox.cpp |
| `Tumbler` | 滚轮选择器 | 自绘 skin #7 Tumbler | B 薄 | 88x96 | 21 | XamlTumbler.cpp |

## 显示控件（7）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `BusyIndicator` | 忙碌指示器 | 自绘 skin #4 Busy | C 仅布局 | 56x56 | 19 | XamlBusyIndicator.cpp |
| `CalendarCtrl` | 日历 | 原生 MFC SysMonthCal32 | B 薄 | 180x160 | 20 | XamlCalendarCtrl.cpp |
| `Hyperlink` | 超链接 | 原生 MFC SysLink | B 薄 | 120x24 | 21 | XamlHyperlink.cpp |
| `ProgressBar` | 进度条 | 原生 MFC msctls\_progress32 | A 基本 | 120x16 | 22 | XamlProgressBar.cpp |
| `RichText` | 富文本 | 原生 MFC RICHEDIT50W | A 基本 | 200x100 | 22 | XamlRichText.cpp |
| `StaticBitmap` | 图片 | 原生 MFC Static (SS\_BITMAP) | B 薄 | 80x80 | 20 | XamlStaticBitmap.cpp |
| `TextBlock` | 文本标签 | 原生 MFC Static | B 薄 | 100x24 | 20 | XamlTextBlock.cpp |

## 数据视图（9）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `GridView` | 网格视图 | 自绘 skin #18 GridView | S 完整 | 320x160 | 24 | XamlGridView.cpp |
| `Page` | 页面 | 自绘 skin #17 Pager（XamlPageView） | B 薄 | 360x160 | 21 | XamlPage.cpp |
| `PageIndicator` | 页指示器 | 自绘 skin #5 Pages | B 薄 | 160x22 | 21 | XamlPageIndicator.cpp |
| `PathView` | 路径视图 | 自绘 skin #19 PathView | B 薄 | 320x140 | 21 | XamlPathView.cpp |
| `StackView` | 栈式视图 | 自绘 skin #17 Pager（XamlPageView 子类） | B 薄 | 360x160 | 21 | XamlStackView.h |
| `SwipeView` | 滑动视图 | 自绘 skin #17 Pager（XamlPageView 子类） | B 薄 | 360x160 | 21 | XamlSwipeView.h |
| `TableView` | 数据表格 | 自绘 skin #9 Table（独立控件套装） | A 基本 | 160x32 | 23 | XamlTableView.cpp |
| `TreeTable` | 树形表格 | 自绘 skin #10 Tree | B 薄 | 320x140 | 21 | XamlTreeView.cpp |
| `VirtualList` | 虚拟列表 | 自绘 skin #16 ListView（虚拟化） | A 基本 | 240x140 | 23 | XamlListView.cpp |

## 功能条（2）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `MenuStrip` | 菜单栏 | 自绘 skin #14 MenuBar | B 薄 | 360x28 | 21 | XamlMenuBar.cpp |
| `ToolStrip` | 工具栏 | 自绘 skin #11 ToolBar | B 薄 | 360x32 | 21 | XamlToolBar.cpp |

## 浮层（3）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `Menu` | 弹出菜单 | 自绘 skin #13 Menu | B 薄 | 200x120 | 21 | XamlMenu.cpp |
| `Popup` | 浮层 | 自绘 skin #15 Popup | A 基本 | 320x160 | 22 | XamlPopup.cpp |
| `ToolTip` | 提示气泡 | 自绘 skin #12 ToolTip | B 薄 | 160x24 | 20 | XamlToolTip.cpp |

## 画布（2）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `PowerCanvas` | 配电规划画布 | 宿主自绘画布（配电规划图） | B 薄 | 420x260 | 20 | XamlPowerCanvas.cpp |
| `QCanvas` | 通用画布 | 自绘 skin #20 QCanvas（命令脚本画布） | B 薄 | 420x200 | 21 | XamlQCanvas.cpp |

## 绘制图元（4）

| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |
|---|---|---|---|---|---|---|
| `Arc` | 圆弧 | — | — | 140x100 | — | — |
| `Ellipse` | 椭圆 | — | — | 120x120 | — | — |
| `Line` | 直线 | — | — | 120x2 | — | — |
| `Rectangle` | 矩形 | — | — | 160x90 | — | — |

## 合计

共 **56** 个标签：51 个主控件 + 5 个绘制图元（Line / Rectangle / Ellipse / Arc / Border）。

不在本表的旧别名（已淘汰，勿再使用）：`TrackBar`（Slider 的旧名）、`Label`（TextBlock）、`Edit`（TextBox）、`Progress`（ProgressBar）、`TreeView`（TreeCtrl）、`ListView`（ListCtrl）。
