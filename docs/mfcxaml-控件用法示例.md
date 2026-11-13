# MFCXaml 控件用法示例

> 示例以 **UIDesign 可直接打开的文档** 为准，放在 `UIDesign\samples\` 下，用主标签、不用别名：
> `01_basic_controls.xaml`（按钮/文本/选择/取值/指示，32 个控件）、
> `02_data_controls.xaml`（表格/树/列表/虚拟化/分页，16 个）、
> `03_overlay_canvas.xaml`（菜单/工具条/浮层/画布/日期/容器，17 个）。
> 三份合计覆盖 `prop_schema()` 的 51 个主标签。
>
> 打开方式：`G:\PyArx\X64\Release\UIDesign.exe "G:\PyArx\UIDesign\samples\01_basic_controls.xaml"`
> （也可以在设计器里「文件 → 打开 XAML…」）。
>
> 校验：`python tools\check_design_samples.py`（逐份 guiload 建窗并核对主标签覆盖，当前 65 个控件实例、51/51 PASS）；
> `python tools\check_control_catalog.py`（核对控件箱目录与 schema 对齐，目录 56 条 = 51 主标签 + 5 个绘制图元）。
>
> 下面按控件族给出等价的最小 XAML 片段，便于直接抄进自己的文档；完整版看三份示例文档。

## 零、先看 UIDesign 的用法

设计器左侧「控件箱」直接拖控件，右侧「属性编辑器」改值，底部是 XAML 源码；
`预览` 按钮会把当前文档交给 XamlViewer 真渲染。文档本身仍是标准 XAML，保存后
`guiload` 能直接加载运行。控件箱条目来自 `UIDesign\control_catalog.xml`，
目录按 `prop_schema()` 生成，缺条目时设计器会提示“未知标签”。

> 注意：`TreeCtrl` / `TreeTable` 的 `Nodes` 在画布上显示为占位，树内容请用
> `addTreeNode(id, 文本, parentIndex)` 在 Python 侧灌（这是运行时已验证的写法）。

## 一、按钮与勾选

覆盖：`Button` / `ToggleButton` / `CheckBox` / `RadioButton` / `GroupBox` / `BitmapButton` / `StaticBitmap`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="按钮族" Width="420" Height="340">
  <XStackPanel Margin="12" Spacing="8">
    <XButton x:Name="Run" Text="运行" Height="32" Click="on_run"/>
    <XToggleButton x:Name="Night" Text="夜间模式" Height="32" CheckedChanged="on_night"/>
    <XCheckBox x:Name="Agree" Text="同意条款" Height="26" CheckedChanged="on_agree"/>
    <XRadioButton x:Name="PlanA" Text="方案 A" Height="26"/>
    <XGroupBox x:Name="Group1" Header="参数组" Height="70"/>
    <XBitmapButton x:Name="Export" Text="导出" Height="32"/>
    <XStaticBitmap x:Name="Preview" Height="56"/>
  </XStackPanel>
</XWindow>
```

要点：按钮类文字统一走 `Text`（`Content` 是历史别名，不再用）；`Checked` 可读写，
`ToggleButton` / `CheckBox` 用 `CheckedChanged`，`RadioButton` 同。

## 二、文本输入与展示

覆盖：`TextBlock` / `TextBox` / `RichText` / `CodeEditor` / `Hyperlink`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="文本族" Width="460" Height="380">
  <XStackPanel Margin="12" Spacing="8">
    <XTextBlock x:Name="Title" Text="回路参数" Height="26" FontSize="14" FontWeight="Bold"/>
    <XTextBox x:Name="Name" Text="WL-AT1-1" Placeholder="请输入回路编号" MaxLength="32" Height="30"
             TextChanged="on_name"/>
    <XTextBox x:Name="Note" Multiline="true" Height="70" Text="多行备注"/>
    <XTextBox x:Name="Code" Text="只读内容" IsReadOnly="true" Height="30"/>
    <XRichText x:Name="Rtf" Text="富文本段落" Bold="true" Italic="false" FontSize="11"
              Foreground="#334155" Height="80"/>
    <XCodeEditor x:Name="Src" Syntax="xaml" Text="&lt;Button Text=&quot;运行&quot;/&gt;" Height="90"/>
    <XHyperlink x:Name="Help" Text="查看规范" Url="https://example.com/doc" Height="26"
               Click="on_help"/>
  </XStackPanel>
</XWindow>
```

要点：`RichText` 的字号与前景色就是通用外观字段 `FontSize` / `Foreground`（v0.10.17 起不再重复声明）；
`CodeEditor` 的 `Syntax` 取 `xaml` / `python`。

## 三、选择类

覆盖：`ComboBox` / `Choice` / `ListBox`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="选择族" Width="420" Height="300">
  <XStackPanel Margin="12" Spacing="8">
    <XComboBox x:Name="Cable" Items="YJV22-4×35|YJV22-4×25|VV-3×10" SelectedIndex="0"
              DropDownStyle="DropDown" Height="28" SelectionChanged="on_cable"/>
    <XChoice x:Name="Zone" Items="A 区|B 区|C 区" Height="28"/>
    <XListBox x:Name="Loads" Items="照明 12kW|动力 30kW|空调 8kW" SelectionMode="Single" Height="110"/>
  </XStackPanel>
</XWindow>
```

要点：`Items` 用 `|` 分隔；`SelectedIndex` 读写；`ComboBox` 还带 `Text`（可编辑）与 `DropDownStyle`。

## 四、数值与开关

覆盖：`ProgressBar` / `Slider` / `SpinBox` / `RangeSlider` / `Dial` / `Tumbler` / `ScrollBar` / `Switch`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="取值族" Width="440" Height="420">
  <XStackPanel Margin="12" Spacing="8">
    <XProgressBar x:Name="Pct" Minimum="0" Maximum="100" Value="65" Height="22"/>
    <XSlider x:Name="Level" Minimum="0" Maximum="100" Value="45" Height="34" ValueChanged="on_level"/>
    <XSpinBox x:Name="Count" Minimum="1" Maximum="64" Value="12" Height="30"/>
    <XRangeSlider x:Name="Band" Minimum="0" Maximum="100" LowValue="25" HighValue="75" Height="34"/>
    <XDial x:Name="Knob" Minimum="0" Maximum="100" Value="40" Height="64"/>
    <XTumbler x:Name="Hour" Count="24" SelectedIndex="3" Height="72"/>
    <XScrollBar x:Name="Bar" Minimum="0" Maximum="200" Value="35" PageSize="20"
               Orientation="Vertical" Height="46"/>
    <XSwitch x:Name="Power" Checked="true" Height="30" CheckedChanged="on_power"/>
  </XStackPanel>
</XWindow>
```

要点：这一类都遵循 `Minimum` / `Maximum` / `Value` 口径（`RangeSlider` 用 `LowValue` / `HighValue`，
`Tumbler` / `PageIndicator` 用 `Count` + `SelectedIndex`）。

## 五、列表与表格

覆盖：`ListCtrl` / `TreeCtrl` / `GridCtrl` / `TableView` / `TreeTable` / `VirtualList` / `GridView` / `PathView`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="列表族" Width="620" Height="560">
  <XStackPanel Margin="12" Spacing="8">
    <XListCtrl x:Name="Files" Columns="名称:140|类型:90" Rows="图框	A3|标注	箭头" Height="90"/>
    <XTreeCtrl x:Name="Tree1" Nodes="项目根|图纸 A|图纸 B|图层组" Height="90"/>
    <XGridCtrl x:Name="Grid1" Columns="名称:120|数值:80" Rows="甲	12|乙	34" Height="90"/>
    <XTableView x:Name="Table1" Columns="名称:120|数值:80|状态:70"
               Rows="甲	30	正常|乙	12	警告|丙	21	正常" SelectedIndex="1" Height="110"
               SelectionChanged="on_row"/>
    <XTreeTable x:Name="Hier" Nodes="项目根|图纸 A|图层组" SelectedIndex="1" Height="90"/>
    <XVirtualList x:Name="VList" Items="第一行|第二行|第三行|第四行" ItemHeight="24"
                 SelectedIndex="0" Height="90"/>
    <XGridView x:Name="Tiles" Items="回路 1|回路 2|回路 3|回路 4" CellWidth="80" CellHeight="56"
              SelectedIndex="0" Height="90"/>
    <XPathView x:Name="Cover" Items="方案 A|方案 B|方案 C" SelectedIndex="1" Height="90"/>
  </XStackPanel>
</XWindow>
```

要点：列/行用 `名称:宽度` 与制表符分隔（`\t`）；`TableView.RowCount`、`VirtualList.ItemCount`、
`GridView.ItemCount` 只读，用于回读。

## 六、容器与布局件

覆盖：`Panel` / `ScrolledWindow` / `Splitter` / `StaticLine` / `Flickable`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="容器族" Width="460" Height="420">
  <XStackPanel Margin="12" Spacing="8">
    <XPanel x:Name="Host" Height="70"/>
    <XStaticLine x:Name="Sep1" Height="10"/>
    <XSplitter x:Name="Split" Height="16"/>
    <XScrolledWindow x:Name="Scroll" ScrollMin="0" ScrollMax="400" Height="90"/>
    <XFlickable x:Name="Flick" OffsetX="0" OffsetY="0" Height="110"/>
  </XStackPanel>
</XWindow>
```

要点：容器子控件写在容器标签内（`<XPanel><XTextBlock .../></XPanel>`）；
`Flickable.OffsetX/OffsetY` 是 v0.10.17 新增的读写视口偏移。

## 七、页与导航

覆盖：`TabControl` / `StackView` / `SwipeView` / `Page` / `PageIndicator`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="页族" Width="460" Height="420">
  <XStackPanel Margin="12" Spacing="8">
    <XTabControl x:Name="Tabs" Pages="常规|高级|关于" Height="110"/>
    <XStackView x:Name="Stack" SelectedIndex="0" PageCount="3" Height="90"/>
    <XSwipeView x:Name="Swipe" SelectedIndex="1" PageCount="2" Height="90"/>
    <XPage x:Name="Wizard" SelectedIndex="0" PageCount="2" Height="70"/>
    <XPageIndicator x:Name="Dots" Count="3" SelectedIndex="1" Height="22"/>
  </XStackPanel>
</XWindow>
```

要点：`StackView` / `SwipeView` / `Page` 是同一实现（`XamlPageView`）的三个语义名，
`SelectedIndex` + `PageCount` 同口径；`PageIndicator` 用 `Count`。

## 八、菜单、工具条与浮层

覆盖：`Menu` / `MenuStrip` / `ToolStrip` / `ToolTip` / `Popup`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="浮层族" Width="460" Height="380">
  <XStackPanel Margin="12" Spacing="8">
    <XMenuStrip x:Name="Top" Items="文件|编辑|视图" SelectedIndex="0" Height="28"/>
    <XToolStrip x:Name="Bar1" Items="新建|打开|保存|运行" SelectedIndex="2" Height="32"
               SelectionChanged="on_tool"/>
    <XMenu x:Name="Ctx" Items="复制|粘贴|删除" Height="60"/>
    <XToolTip x:Name="Hint" Text="鼠标停留提示" Height="24"/>
    <XPopup x:Name="Sheet" Title="确认" Content="保存后关闭？" Height="90"/>
  </XStackPanel>
</XWindow>
```

要点：`Menu` 是上下文菜单（不占版面时由 `showMenu` 弹出），`MenuStrip` 是顶部菜单条；
`Popup` 与 `Dialog` / `Drawer` 同一实现。

## 九、画布

覆盖：`PowerCanvas` / `QCanvas`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="画布族" Width="520" Height="360">
  <XStackPanel Margin="12" Spacing="8">
    <XPowerCanvas x:Name="Plan" Data="{}" Height="200" Click="on_plan_click"/>
    <XQCanvas x:Name="Draw" Height="120"/>
  </XStackPanel>
</XWindow>
```

要点：`PowerCanvas` 吃配电规划模型（`Data` 是规划 JSON，实际运行用 `loadPowerPlan` 灌）；
`QCanvas` 吃绘制命令（`Commands`）或脚本（`Script`），单击/双击/滚轮/键事件齐全。

## 十、日期

覆盖：`DatePicker` / `CalendarCtrl`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="日期族" Width="420" Height="320">
  <XStackPanel Margin="12" Spacing="8">
    <XDatePicker x:Name="Day" Value="2026-11-13" Height="30" ValueChanged="on_day"/>
    <XCalendarCtrl x:Name="Cal" Value="2026-11-13" Height="180" SelectionChanged="on_cal"/>
  </XStackPanel>
</XWindow>
```

要点：两者 `Value` 都是 `YYYY-MM-DD` 文本（`CalendarCtrl.Value` 是 v0.10.17 补的，
宿主走 `MCM_GETCURSEL` / `MCM_SETCURSEL`）。

## 十一、指示件

覆盖：`BusyIndicator`

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="指示" Width="320" Height="200">
  <XStackPanel Margin="12" Spacing="8">
    <XBusyIndicator x:Name="Busy" Height="56"/>
  </XStackPanel>
</XWindow>
```

要点：`BusyIndicator` 现在的自有属性是 0（只有外观/布局字段），动画由 skin #4 定时器驱动；
要加 `Running` / `Interval` 得先给 skinbox 加状态接口。

## 十二、组合示例：回路参数卡（含回调）

覆盖落点：`Panel` + `TextBlock` + `TextBox` + `ComboBox` + `Slider` + `Switch` +
`TableView` + `Button` + `PageIndicator`（组合控件由 XAML 拼，不硬造新类）。

```xml
<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="回路参数卡" Width="640" Height="520">
  <XStackPanel Margin="14" Spacing="10">
    <XTextBlock x:Name="Head" Text="AT1-1 回路参数" Height="28" FontSize="15" FontWeight="Bold"/>

    <XPanel x:Name="Row1" Height="34">
      <XTextBox x:Name="Code" Text="WL-AT1-1" Left="0" Top="2" Width="220" Height="30"/>
      <XButton x:Name="Apply" Text="应用" Left="232" Top="2" Width="90" Height="30"
              Click="on_apply"/>
    </XPanel>

    <XComboBox x:Name="Cable" Items="YJV22-4×35|YJV22-4×25" SelectedIndex="0" Height="28"/>
    <XSlider x:Name="Load" Minimum="0" Maximum="100" Value="11" Height="34" ValueChanged="on_load"/>
    <XSwitch x:Name="Energized" Checked="true" Height="28"/>

    <XTableView x:Name="Detail" Columns="参数:140|取值:90|单位:60"
               Rows="相数	1	-
额定电压	380	V
计算电流	21.4	A" Height="120" SelectionChanged="on_detail"/>

    <XPageIndicator x:Name="Dots" Count="3" SelectedIndex="0" Height="20"/>
  </XStackPanel>
</XWindow>
```

Python 侧接线（回调名在 XAML 里声明，函数从 `context` 里解析；也可以加载后再绑定）：

```python
import os
import PyMfcXaml as M

names = {}

def on_apply(comp, name):
    print('应用', comp, name)

def on_load(comp, name):
    print('负载变化', name)

def on_detail(comp, name):
    print('选中行', name)

xml = r'G:\PyArx\examples\circuit_card.xaml'
win = M.guiload(xml, context=globals(), names=names)
win.show()
M.pump(300)

# 加载后改属性 / 绑回调同样可以
win.setXamlValue('Code', 'WL-AT1-2')
c = names.get('Apply')
if c is not None:
    win.setControlCallback(c, 'click', on_apply) if hasattr(win, 'setControlCallback') else None

M.pump(200)
win.close() if hasattr(win, 'close') else None
```

要点：复合卡片走 XAML 组合（`Panel` 内绝对定位），属性都落在子控件上；
回调统一走 Python（`Click="on_xxx"` 由 `context` 解析，或加载后按控件名绑）。

## 十三、示例覆盖核对

| 组 | 控件 |
|---|---|
| 一 按钮与勾选 | Button / ToggleButton / CheckBox / RadioButton / GroupBox / BitmapButton / StaticBitmap |
| 二 文本 | TextBlock / TextBox / RichText / CodeEditor / Hyperlink |
| 三 选择 | ComboBox / Choice / ListBox |
| 四 取值 | ProgressBar / Slider / SpinBox / RangeSlider / Dial / Tumbler / ScrollBar / Switch |
| 五 列表表格 | ListCtrl / TreeCtrl / GridCtrl / TableView / TreeTable / VirtualList / GridView / PathView |
| 六 容器 | Panel / ScrolledWindow / Splitter / StaticLine / Flickable |
| 七 页 | TabControl / StackView / SwipeView / Page / PageIndicator |
| 八 浮层 | Menu / MenuStrip / ToolStrip / ToolTip / Popup |
| 九 画布 | PowerCanvas / QCanvas |
| 十 日期 | DatePicker / CalendarCtrl |
| 十一 指示 | BusyIndicator |

合计 51 个主标签，与 `prop_schema()` 的控件数一致（`tools\check_design_samples.py` 会断言这一点，
并在 UIDesign 里逐份打开核对渲染）。
