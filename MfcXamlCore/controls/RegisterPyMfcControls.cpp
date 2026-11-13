#include "RegisterPyMfcControls.h"

#include "XamlCore.h"
#include "XamlMfcControlBase.h"
#include "XamlButton.h"
#include "XamlTextBlock.h"
#include "XamlTextBox.h"
#include "XamlCodeEditor.h"
#include "XamlListBox.h"
#include "XamlCheckBox.h"
#include "XamlRadioButton.h"
#include "XamlGroupBox.h"
#include "XamlComboBox.h"
#include "XamlProgressBar.h"
#include "XamlSlider.h"
#include "XamlExtraControls.h"

#include <functional>
#include <vector>

// 注册单个控件：主标签 + 别名 -> 同一工厂。用 wchar 宽字符 key。
static void RegisterOne(XamlControlRegistry& reg,
                        std::function<IXamlControl*()> factory,
                        const std::vector<const wchar_t*>& names)
{
    for (const wchar_t* name : names)
    {
        if (reg.isKnown(name))
            continue;
        reg.registerFactory(name, factory);
    }
}

void RegisterPyMfcControls()
{
    auto& reg = XamlControlRegistry::instance();

    RegisterOne(reg, []() -> IXamlControl* { return new XamlButton(); },
                {L"Button", L"button", L"BTN"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTextBlock(); },
                {L"TextBlock", L"textblock", L"Label", L"label"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTextBox(); },
                {L"TextBox", L"textbox", L"Edit", L"edit"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListBox(); },
                {L"ListBox", L"listbox"});
    // 代码编辑器（独立控件）：Syntax 取 xaml / python，带行号槽与语法着色
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCodeEditor(); },
                {L"CodeEditor", L"codeeditor", L"CodeView", L"codeview", L"XmlEditor"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCheckBox(); },
                {L"CheckBox", L"checkbox", L"Checkbox"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRadioButton(); },
                {L"RadioButton", L"radiobutton", L"Radio", L"radio"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGroupBox(); },
                {L"GroupBox", L"groupbox", L"Group", L"group"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlComboBox(); },
                {L"ComboBox", L"combobox", L"Combo", L"combo"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlProgressBar(); },
                {L"ProgressBar", L"progressbar", L"Progress", L"progress"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSlider(); },
                {L"Slider", L"slider"});
    // 扩展控件（对齐 wxFormBuilder 常见控件）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTabControl(); },
                {L"TabControl", L"tabcontrol", L"TabWidget", L"tabwidget"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTreeCtrl(); },
                {L"TreeCtrl", L"treectrl", L"TreeView", L"tree"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListCtrl(); },
                {L"ListCtrl", L"listctrl", L"ListView", L"listctrl_view"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlChoice(); },
                {L"Choice", L"choice"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlDatePicker(); },
                {L"DatePicker", L"datepicker", L"DatePickerCtrl"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPanel(); },
                {L"Panel", L"panel"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSplitter(); },
                {L"Splitter", L"splitter", L"SplitterWindow"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStaticLine(); },
                {L"StaticLine", L"staticline"});
    // 配电规划图画布（自绘：缩放平移 + 卡片命中）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPowerCanvas(); },
                {L"PowerCanvas", L"powercanvas", L"PlanCanvas"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlHyperlink(); },
                {L"Hyperlink", L"hyperlink", L"HyperlinkCtrl"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRichText(); },
                {L"RichText", L"richtext", L"RichTextCtrl"});
    // 第二批扩展控件
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToggleButton(); },
                {L"ToggleButton", L"togglebutton", L"ToggleBtn"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlBitmapButton(); },
                {L"BitmapButton", L"bitmapbutton", L"BitmapBtn"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStaticBitmap(); },
                {L"StaticBitmap", L"staticbitmap"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGrid(); },
                {L"GridCtrl", L"gridctrl", L"DataGrid"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlScrolledWindow(); },
                {L"ScrolledWindow", L"scrolledwindow"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCalendarCtrl(); },
                {L"CalendarCtrl", L"calendar", L"Calendar"});
    // QML 对齐（第三批）：自建画布小控件
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSwitch(); },
                {L"Switch", L"switch", L"ToggleSwitch"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSpinBox(); },
                {L"SpinBox", L"spinbox", L"NumberBox", L"NumericUpDown"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRangeSlider(); },
                {L"RangeSlider", L"rangeslider", L"SliderRange"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlBusyIndicator(); },
                {L"BusyIndicator", L"busyindicator", L"Spinner", L"ActivityIndicator"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPageIndicator(); },
                {L"PageIndicator", L"pageindicator", L"PagerDots"});
    // QML 对齐（第四批）：Dial / Tumbler / 独立 ScrollBar
    RegisterOne(reg, []() -> IXamlControl* { return new XamlDial(); },
                {L"Dial", L"dial", L"Knob", L"Gauge"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTumbler(); },
                {L"Tumbler", L"tumbler", L"Wheel"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlScrollBar(); },
                {L"ScrollBar", L"scrollbar", L"VScrollBar", L"HScrollBar"});
    // QML 对齐（第五批）：表格类控件 + 工具栏
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTableView(); },
                {L"TableView", L"tableview", L"DataTable", L"GridTable"});
    // 注意：`TreeView` 已经是原生 SysTreeView32 的别名，`ToolBar`/`toolbar` 是布局容器，
    // 所以这两个自绘控件用 TreeTable / ToolStrip 作主名，不去抢旧别名。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTreeView(); },
                {L"TreeTable", L"treetable", L"HierarchicalTable"});
    // 注意：`ToolBar` / `toolbar` 这个标签归布局层（工具栏容器，用来横向排子控件），
    // 所以自绘的工具条按钮条用 ToolStrip 这个名字，避免把布局容器顶掉。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToolBar(); },
                {L"ToolStrip", L"toolstrip", L"ToolButtonBar"});

    // QML 对齐（第六批）：浮层族
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToolTip(); },
                {L"ToolTip", L"tooltip", L"Hint"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlMenu(); },
                {L"Menu", L"menu", L"ContextMenu", L"PopupMenu"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlMenuBar(); },
                {L"MenuStrip", L"menustrip", L"MenuBarControl", L"TopMenuBar"});
    // Popup / Dialog / Drawer 共用同一个实现（标题条 + 正文 + 点正文关闭）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"Popup", L"popup", L"Overlay"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"Dialog", L"dialog", L"MessageBoxX"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"Drawer", L"drawer", L"SideSheet"});

    // QML 对齐（第七批）：虚拟化列表（ListModel + delegate）
    // 注意：`ListView` 已是原生 ListCtrl 的别名，这里用 VirtualList 作主名。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListView(); },
                {L"VirtualList", L"virtuallist", L"ListViewV", L"ModelList"});

    // QML 对齐（第八批）：页容器（Page / StackView / SwipeView）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStackView(); },
                {L"StackView", L"stackview", L"PageView", L"CarouselView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSwipeView(); },
                {L"SwipeView", L"swipeview", L"PageSwipe"});
    // QML 对齐（第九批）：网格委托 / 路径委托
    // 注意 GridView 不能用 "Grid" 当别名（Grid 是布局容器名，会被容器先吃掉）。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGridView(); },
                {L"GridView", L"gridview", L"TileView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPathView(); },
                {L"PathView", L"pathview", L"CoverFlow"});
    // 画命令画布：画布就是画布。主名 QCanvas，正名 DrawCanvas / PaintCanvas，
    // 布局容器另有正名 AbsolutePanel（Canvas 是它的历史别名，不再指画布）。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlQCanvas(); },
                {L"QCanvas", L"qcanvas", L"DrawCanvas", L"drawcanvas", L"PaintCanvas",
                 L"paintcanvas", L"CanvasSurface", L"canvassurface", L"Painter"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPage(); },
                {L"Page", L"PagedView", L"WizardPage"});
    // QML 对齐（第十批）：可拖拽内容视口（惯性 + 触边回弹）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlFlickable(); },
                {L"Flickable", L"flickable", L"FlickArea"});
}
