#include "RegisterPyMfcControls.h"

#include "XmlCore.h"
#include "XmlMfcControlBase.h"
#include "XmlButton.h"
#include "XmlTextBlock.h"
#include "XmlTextBox.h"
#include "XmlListBox.h"
#include "XmlCheckBox.h"
#include "XmlRadioButton.h"
#include "XmlGroupBox.h"
#include "XmlComboBox.h"
#include "XmlProgressBar.h"
#include "XmlSlider.h"
#include "XmlExtraControls.h"
#include "XmlFlexPanel.h"
#include "XmlFrame.h"
#include "XmlDialog.h"
#include "XmlRoundButton.h"
#include "XmlDelayButton.h"
#include "XmlToolButton.h"
#include "XmlToolSeparator.h"
#include "XmlTabButton.h"
#include "XmlScrollIndicator.h"
#include "XmlMenuItem.h"
#include "XmlMenuSeparator.h"
#include "XmlItemView.h"
#include "XmlView3D.h"
#include "XmlRibbon.h"
#include "XmlFileTab.h"
#include "XmlShxText.h"

#include <functional>
#include <vector>

// 注册单个控件：主标签 + 别名 -> 同一工厂。用 wchar 宽字符 key。
static void RegisterOne(XmlControlRegistry& reg,
                        std::function<IXmlControl*()> factory,
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
    auto& reg = XmlControlRegistry::instance();

    RegisterOne(reg, []() -> IXmlControl* { return new XmlButton(); },
                {L"XButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTextBlock(); },
                {L"XLabel"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTextBox(); },
                {L"XTextField"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlListBox(); },
                {L"XListBox"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlCheckBox(); },
                {L"XCheckBox"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlRadioButton(); },
                {L"XRadioButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlGroupBox(); },
                {L"XGroupBox"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlComboBox(); },
                {L"XComboBox"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlProgressBar(); },
                {L"XProgressBar"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlSlider(); },
                {L"XSlider"});
    // 扩展控件（对齐 wxFormBuilder 常见控件）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTabControl(); },
                {L"XTabBar"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTreeCtrl(); },
                {L"XTreeView"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlListCtrl(); },
                {L"XListCtrl"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlChoice(); },
                {L"XChoice"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlDatePicker(); },
                {L"XDatePicker"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlPanel(); },
                {L"XPane"});
    // Control-level flex container: it owns the child list and lays children out itself
    // (flex-direction / flex-wrap / justify-content / align-items / gap / padding).
    RegisterOne(reg, []() -> IXmlControl* { return new XmlFlexPanel(); },
                {L"XFlexPanel", L"FlexPanel", L"FlexBox", L"Row", L"Column"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlSplitter(); },
                {L"XSplitView"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlStaticLine(); },
                {L"XDivider"});
    // 绘图画布（DWG 为主，兼容已算坐标的布局输入）（自绘：缩放平移 + 卡片命中）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlDwgCanvas(); },
                {L"XDwgCanvas"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlHyperlink(); },
                {L"XHyperlink"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlRichText(); },
                {L"XTextArea"});
    // 第二批扩展控件
    RegisterOne(reg, []() -> IXmlControl* { return new XmlToggleButton(); },
                {L"XToggleButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlBitmapButton(); },
                {L"XIconButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlStaticBitmap(); },
                {L"XImage"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlGrid(); },
                {L"XGridLayout"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlScrolledWindow(); },
                {L"XScrollView"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlCalendarCtrl(); },
                {L"XMonthGrid"});
    // HTML 对齐（第三批）：自建画布小控件
    RegisterOne(reg, []() -> IXmlControl* { return new XmlSwitch(); },
                {L"XSwitch"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlSpinBox(); },
                {L"XSpinBox"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlRangeSlider(); },
                {L"XRangeSlider"});
    // HTML 对齐（第四批）：Dial / Tumbler / 独立 ScrollBar
    RegisterOne(reg, []() -> IXmlControl* { return new XmlDial(); },
                {L"XDial"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTumbler(); },
                {L"XTumbler"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlScrollBar(); },
                {L"XScrollBar"});
    // HTML 对齐（第五批）：表格类控件 + 工具栏
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTableView(); },
                {L"XTableView"});
    // 注意：`ToolBar` / `toolbar` 这个标签归布局层（工具栏容器，用来横向排子控件），
    // 所以自绘的工具条按钮条用 ToolStrip 这个名字，避免把布局容器顶掉。
    RegisterOne(reg, []() -> IXmlControl* { return new XmlToolBar(); },
                {L"XToolBar"});

    RegisterOne(reg, []() -> IXmlControl* { return new XmlMenu(); },
                {L"XMenu"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlMenuBar(); },
                {L"XMenuBar"});
    // Popup / Dialog / Drawer 共用同一个实现（标题条 + 正文 + 点正文关闭）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlPopup(); },
                {L"XPopup"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlPopup(); },
                {L"XDrawer"});

    // HTML 对齐（第七批）：虚拟化列表（ListModel + delegate）
    // 注意：`ListView` 已是原生 ListCtrl 的别名，这里用 VirtualList 作主名。
    RegisterOne(reg, []() -> IXmlControl* { return new XmlListView(); },
                {L"XVirtualList"});

    // HTML 对齐（第八批）：页容器（Page / StackView / SwipeView）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlStackView(); },
                {L"XStackView"});
    // HTML 对齐（第九批）：网格委托 / 路径委托
    // 注意 GridView 不能用 "Grid" 当别名（Grid 是布局容器名，会被容器先吃掉）。
    RegisterOne(reg, []() -> IXmlControl* { return new XmlGridView(); },
                {L"XGridView"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlPathView(); },
                {L"XPathView"});
    // 画命令画布：画布就是画布。主名 QCanvas，正名 DrawCanvas / PaintCanvas，
    // 布局容器另有正名 AbsolutePanel（Canvas 是它的历史别名，不再指画布）。
    RegisterOne(reg, []() -> IXmlControl* { return new XmlQCanvas(); },
                {L"XQCanvas"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlPage(); },
                {L"XPage"});
    // HTML 对齐（第十批）：可拖拽内容视口（惯性 + 触边回弹）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlFlickable(); },
                {L"XFlickable"});

    // ---- HTML Controls 2 对齐补齐 ----
    RegisterOne(reg, []() -> IXmlControl* { return new XmlFrame(); },
                {L"XFrame"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlDialog(); },
                {L"XDialog"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlRoundButton(); },
                {L"XRoundButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlDelayButton(); },
                {L"XDelayButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlToolButton(); },
                {L"XToolButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlToolSeparator(); },
                {L"XToolSeparator"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlTabButton(); },
                {L"XTabButton"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlScrollIndicator(); },
                {L"XScrollIndicator"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlMenuItem(); },
                {L"XMenuItem"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlMenuSeparator(); },
                {L"XMenuSeparator"});
    RegisterOne(reg, []() -> IXmlControl* { return new XmlItemView(); },
                {L"XItemView"});
    // 三维视图画布：透视投影 + 视口旋转（左键拖动）+ 缩放（滚轮）+ 平移（中键）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlView3D(); },
                {L"XView3D"});
    // 命令功能区（复用控件库 xmlctl::XRibbon：模型/布局/绘制/命中成套）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlRibbon(); },
                {L"XRibbon"});
    // SHX 字体文字：C++ 自带解析器排版成折线，走 DwwCanvas 自绘通路（脱 CAD）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlShxText(); },
                {L"XShxText", L"ShxText"});
    // 多文档文件页签条（复用控件库 xmlctl::FileTab：模型/布局/绘制/命中成套）
    RegisterOne(reg, []() -> IXmlControl* { return new XmlFileTab(); },
                {L"XFileTab", L"filetab"});
}
