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
                {L"XButton"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTextBlock(); },
                {L"XLabel"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTextBox(); },
                {L"XTextField"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListBox(); },
                {L"XListBox"});
    // 代码编辑器（独立控件）：Syntax 取 xaml / python，带行号槽与语法着色
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCodeEditor(); },
                {L"XCodeEditor"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCheckBox(); },
                {L"XCheckBox"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRadioButton(); },
                {L"XRadioButton"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGroupBox(); },
                {L"XGroupBox"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlComboBox(); },
                {L"XComboBox"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlProgressBar(); },
                {L"XProgressBar"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSlider(); },
                {L"XSlider"});
    // 扩展控件（对齐 wxFormBuilder 常见控件）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTabControl(); },
                {L"XTabControl"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTreeCtrl(); },
                {L"XTreeView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListCtrl(); },
                {L"XListCtrl"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlChoice(); },
                {L"XChoice"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlDatePicker(); },
                {L"XDatePicker"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPanel(); },
                {L"XPanel"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSplitter(); },
                {L"XSplitView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStaticLine(); },
                {L"XStaticLine"});
    // 配电规划图画布（自绘：缩放平移 + 卡片命中）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPowerCanvas(); },
                {L"XPowerCanvas"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlHyperlink(); },
                {L"XHyperlink"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRichText(); },
                {L"XTextArea"});
    // 第二批扩展控件
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToggleButton(); },
                {L"XToggleButton"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlBitmapButton(); },
                {L"XBitmapButton"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStaticBitmap(); },
                {L"XImage"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGrid(); },
                {L"XGridCtrl"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlScrolledWindow(); },
                {L"XScrollView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlCalendarCtrl(); },
                {L"XCalendarCtrl"});
    // QML 对齐（第三批）：自建画布小控件
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSwitch(); },
                {L"XSwitch"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSpinBox(); },
                {L"XSpinBox"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlRangeSlider(); },
                {L"XRangeSlider"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlBusyIndicator(); },
                {L"XBusyIndicator"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPageIndicator(); },
                {L"XPageIndicator"});
    // QML 对齐（第四批）：Dial / Tumbler / 独立 ScrollBar
    RegisterOne(reg, []() -> IXamlControl* { return new XamlDial(); },
                {L"XDial"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTumbler(); },
                {L"XTumbler"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlScrollBar(); },
                {L"XScrollBar"});
    // QML 对齐（第五批）：表格类控件 + 工具栏
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTableView(); },
                {L"XTableView"});
    // 注意：`TreeView` 已经是原生 SysTreeView32 的别名，`ToolBar`/`toolbar` 是布局容器，
    // 所以这两个自绘控件用 TreeTable / ToolStrip 作主名，不去抢旧别名。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlTreeView(); },
                {L"XTreeTable"});
    // 注意：`ToolBar` / `toolbar` 这个标签归布局层（工具栏容器，用来横向排子控件），
    // 所以自绘的工具条按钮条用 ToolStrip 这个名字，避免把布局容器顶掉。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToolBar(); },
                {L"XToolBar"});

    // QML 对齐（第六批）：浮层族
    RegisterOne(reg, []() -> IXamlControl* { return new XamlToolTip(); },
                {L"XToolTip"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlMenu(); },
                {L"XMenu"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlMenuBar(); },
                {L"XMenuBar"});
    // Popup / Dialog / Drawer 共用同一个实现（标题条 + 正文 + 点正文关闭）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"XPopup"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"XDialog"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPopup(); },
                {L"XDrawer"});

    // QML 对齐（第七批）：虚拟化列表（ListModel + delegate）
    // 注意：`ListView` 已是原生 ListCtrl 的别名，这里用 VirtualList 作主名。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlListView(); },
                {L"XVirtualList"});

    // QML 对齐（第八批）：页容器（Page / StackView / SwipeView）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlStackView(); },
                {L"XStackView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlSwipeView(); },
                {L"XSwipeView"});
    // QML 对齐（第九批）：网格委托 / 路径委托
    // 注意 GridView 不能用 "Grid" 当别名（Grid 是布局容器名，会被容器先吃掉）。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlGridView(); },
                {L"XGridView"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPathView(); },
                {L"XPathView"});
    // 画命令画布：画布就是画布。主名 QCanvas，正名 DrawCanvas / PaintCanvas，
    // 布局容器另有正名 AbsolutePanel（Canvas 是它的历史别名，不再指画布）。
    RegisterOne(reg, []() -> IXamlControl* { return new XamlQCanvas(); },
                {L"XQCanvas"});
    RegisterOne(reg, []() -> IXamlControl* { return new XamlPage(); },
                {L"XPage"});
    // QML 对齐（第十批）：可拖拽内容视口（惯性 + 触边回弹）
    RegisterOne(reg, []() -> IXamlControl* { return new XamlFlickable(); },
                {L"XFlickable"});
}
