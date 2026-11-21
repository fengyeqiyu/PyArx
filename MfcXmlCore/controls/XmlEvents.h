#pragma once
// XmlEvents —— 控件事件契约（唯一来源）。
//
// 前端（XML）只声明事件名：<Button Click="onOk" />、<ComboBox SelectionChanged="onSel" />；
// 后端（控件实现 + 宿主）负责把原生消息派发成事件并调用回调。
// 回调缺省时事件静默丢弃，绝不报错——事件机制本身必须齐全。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XML_EVENTS_H
#define XML_EVENTS_H

#include <string>
#include <vector>
#include <map>
#include <cwctype>
#include <cmath>

enum class XmlEventKind
{
    Click,            // 按钮按下松开 / 复选、单选、切换按钮状态被用户改变
    DoubleClick,      // 双击
    TextChanged,      // 编辑框、富文本内容变化
    SelectionChanged, // 下拉框、列表框、树、列表控件选中项变化
    ValueChanged,     // 滑块、进度、数值类控件取值变化
    CheckedChanged,   // 复选/单选/开关勾选状态变化
    DropDown,         // 下拉框展开
    Scroll,           // 滚动
    MouseDown,
    MouseUp,
    MouseMove,
    KeyDown,
    KeyUp,
    GotFocus,
    LostFocus,
};

// 内部键（bindEvent 用，小写无空格）
inline const wchar_t* xmlEventKey(XmlEventKind k)
{
    switch (k)
    {
    case XmlEventKind::Click: return L"click";
    case XmlEventKind::DoubleClick: return L"dblclick";
    case XmlEventKind::TextChanged: return L"change";
    case XmlEventKind::SelectionChanged: return L"select";
    case XmlEventKind::ValueChanged: return L"value";
    case XmlEventKind::CheckedChanged: return L"checked";
    case XmlEventKind::DropDown: return L"dropdown";
    case XmlEventKind::Scroll: return L"scroll";
    case XmlEventKind::MouseDown: return L"mousedown";
    case XmlEventKind::MouseUp: return L"mouseup";
    case XmlEventKind::MouseMove: return L"mousemove";
    case XmlEventKind::KeyDown: return L"keydown";
    case XmlEventKind::KeyUp: return L"keyup";
    case XmlEventKind::GotFocus: return L"gotfocus";
    case XmlEventKind::LostFocus: return L"lostfocus";
    }
    return L"";
}

// XML 属性名（前端写法）
inline const wchar_t* xmlEventProperty(XmlEventKind k)
{
    switch (k)
    {
    case XmlEventKind::Click: return L"Click";
    case XmlEventKind::DoubleClick: return L"DoubleClick";
    case XmlEventKind::TextChanged: return L"TextChanged";
    case XmlEventKind::SelectionChanged: return L"SelectionChanged";
    case XmlEventKind::ValueChanged: return L"ValueChanged";
    case XmlEventKind::CheckedChanged: return L"CheckedChanged";
    case XmlEventKind::DropDown: return L"DropDown";
    case XmlEventKind::Scroll: return L"Scroll";
    case XmlEventKind::MouseDown: return L"MouseDown";
    case XmlEventKind::MouseUp: return L"MouseUp";
    case XmlEventKind::MouseMove: return L"MouseMove";
    case XmlEventKind::KeyDown: return L"KeyDown";
    case XmlEventKind::KeyUp: return L"KeyUp";
    case XmlEventKind::GotFocus: return L"GotFocus";
    case XmlEventKind::LostFocus: return L"LostFocus";
    }
    return L"";
}

// 属性名 -> kind（不区分大小写）。不是事件属性返回 false。
inline bool xmlEventFromProperty(const std::wstring& name, XmlEventKind& out)
{
    std::wstring t = name;
    for (auto& c : t) c = (wchar_t)towlower(c);
    if (t == L"click" || t == L"onclick") { out = XmlEventKind::Click; return true; }
    if (t == L"doubleclick" || t == L"dblclick" || t == L"ondoubleclick") { out = XmlEventKind::DoubleClick; return true; }
    if (t == L"textchanged" || t == L"onchange") { out = XmlEventKind::TextChanged; return true; }
    if (t == L"selectionchanged" || t == L"onselect") { out = XmlEventKind::SelectionChanged; return true; }
    if (t == L"valuechanged" || t == L"onvalue") { out = XmlEventKind::ValueChanged; return true; }
    if (t == L"checkedchanged" || t == L"onchecked") { out = XmlEventKind::CheckedChanged; return true; }
    if (t == L"dropdown" || t == L"ondropdown") { out = XmlEventKind::DropDown; return true; }
    if (t == L"scroll" || t == L"onscroll") { out = XmlEventKind::Scroll; return true; }
    if (t == L"mousedown" || t == L"onmousedown") { out = XmlEventKind::MouseDown; return true; }
    if (t == L"mouseup" || t == L"onmouseup") { out = XmlEventKind::MouseUp; return true; }
    if (t == L"mousemove" || t == L"onmousemove") { out = XmlEventKind::MouseMove; return true; }
    if (t == L"keydown" || t == L"onkeydown") { out = XmlEventKind::KeyDown; return true; }
    if (t == L"keyup" || t == L"onkeyup") { out = XmlEventKind::KeyUp; return true; }
    if (t == L"gotfocus" || t == L"onfocus") { out = XmlEventKind::GotFocus; return true; }
    if (t == L"lostfocus" || t == L"onblur") { out = XmlEventKind::LostFocus; return true; }
    // QML 写法（信号处理器）：onClicked / onCheckedChanged / onTextChanged …
    if (t == L"onclicked") { out = XmlEventKind::Click; return true; }
    if (t == L"ondoubleclicked" || t == L"ondoubletap") { out = XmlEventKind::DoubleClick; return true; }
    if (t == L"oncheckedchanged") { out = XmlEventKind::CheckedChanged; return true; }
    if (t == L"ontextchanged") { out = XmlEventKind::TextChanged; return true; }
    if (t == L"onvaluechanged") { out = XmlEventKind::ValueChanged; return true; }
    if (t == L"onselectionchanged") { out = XmlEventKind::SelectionChanged; return true; }
    if (t == L"onpressed") { out = XmlEventKind::MouseDown; return true; }
    if (t == L"onreleased") { out = XmlEventKind::MouseUp; return true; }
    if (t == L"onpositionchanged") { out = XmlEventKind::MouseMove; return true; }
    if (t == L"onactivated") { out = XmlEventKind::GotFocus; return true; }
    if (t == L"ondeactivated") { out = XmlEventKind::LostFocus; return true; }
    if (t == L"onwheel") { out = XmlEventKind::Scroll; return true; }

    return false;
}

// 每种控件"该有的事件"（S2）：由控件类型名给出缺省集合。
// 数据驱动（一张表），不再用 if 链逐个类型名分支；控件类可用 events() 覆盖。
//
// 类型名用小写短名，与各 XmlXxx 构造函数里传给 XmlMfcControlBase 的一致。
// 契约总表（全项目唯一一份）：控件契约名 -> 该控件该有的事件。
// 键 = 控件 type()（= 注册标签小写），一个控件一个名字，不写别名。
inline const std::map<std::wstring, std::vector<XmlEventKind>>& xmlEventTable()
{
    static const std::vector<XmlEventKind> kPointer = {
        XmlEventKind::Click, XmlEventKind::DoubleClick, XmlEventKind::MouseDown,
        XmlEventKind::MouseUp, XmlEventKind::MouseMove, XmlEventKind::KeyDown,
        XmlEventKind::KeyUp, XmlEventKind::GotFocus, XmlEventKind::LostFocus
    };
    static const std::vector<XmlEventKind> kText = {
        XmlEventKind::TextChanged, XmlEventKind::KeyDown, XmlEventKind::KeyUp,
        XmlEventKind::GotFocus, XmlEventKind::LostFocus, XmlEventKind::Click,
        XmlEventKind::DoubleClick
    };
    static const std::vector<XmlEventKind> kSelect = {
        XmlEventKind::SelectionChanged, XmlEventKind::Click, XmlEventKind::DoubleClick,
        XmlEventKind::Scroll, XmlEventKind::GotFocus, XmlEventKind::LostFocus
    };
    static const std::vector<XmlEventKind> kValue = {
        XmlEventKind::ValueChanged, XmlEventKind::Scroll, XmlEventKind::GotFocus,
        XmlEventKind::LostFocus, XmlEventKind::Click
    };
    static const std::vector<XmlEventKind> kCheck = {
        XmlEventKind::Click, XmlEventKind::CheckedChanged, XmlEventKind::DoubleClick,
        XmlEventKind::GotFocus, XmlEventKind::LostFocus
    };
    static const std::vector<XmlEventKind> kFairly = {
        XmlEventKind::Click, XmlEventKind::SelectionChanged, XmlEventKind::GotFocus,
        XmlEventKind::LostFocus
    };
    static const std::vector<XmlEventKind> kCanvas = {
        XmlEventKind::Click, XmlEventKind::DoubleClick, XmlEventKind::MouseDown,
        XmlEventKind::MouseUp, XmlEventKind::MouseMove, XmlEventKind::Scroll,
        XmlEventKind::KeyDown, XmlEventKind::KeyUp, XmlEventKind::GotFocus,
        XmlEventKind::LostFocus
    };

    // 键 = 控件契约名（= 注册标签的小写形式），一个控件一个名字，不写别名
    static const std::map<std::wstring, std::vector<XmlEventKind>> kTable = {
        // 按钮族
        {L"button", kPointer}, {L"togglebutton", kCheck}, {L"iconbutton", kPointer},
        {L"checkbox", kCheck},
        {L"radiobutton", {XmlEventKind::Click, XmlEventKind::CheckedChanged,
                          XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        // 文本族
        {L"textfield", kText}, {L"textarea", kText},
        // 选择族
        {L"combobox", {XmlEventKind::SelectionChanged, XmlEventKind::TextChanged,
                       XmlEventKind::DropDown, XmlEventKind::DoubleClick,
                       XmlEventKind::KeyDown, XmlEventKind::KeyUp,
                       XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        {L"choice", {XmlEventKind::SelectionChanged, XmlEventKind::DropDown,
                     XmlEventKind::DoubleClick, XmlEventKind::KeyDown,
                     XmlEventKind::KeyUp, XmlEventKind::GotFocus,
                     XmlEventKind::LostFocus}},
        {L"listbox", kSelect}, {L"listctrl", kSelect}, {L"treeview", kSelect},
        {L"tabbar", kFairly}, {L"tableview", kSelect},
        {L"gridlayout", kSelect}, {L"gridview", kSelect}, {L"pathview", kSelect},
        {L"virtuallist", kSelect}, {L"stackview", kFairly},
        {L"page", kFairly},
        // 取值族
        {L"slider", kValue}, {L"rangeslider", kValue}, {L"progressbar", kValue},
        {L"dial", kValue}, {L"tumbler", kValue}, {L"spinbox", kValue},
        {L"scrollbar", {XmlEventKind::ValueChanged, XmlEventKind::Scroll,
                        XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        {L"switch", {XmlEventKind::Click, XmlEventKind::CheckedChanged,
                     XmlEventKind::ValueChanged, XmlEventKind::GotFocus,
                     XmlEventKind::LostFocus}},
        // 静态/容器
        {L"label", kPointer}, {L"groupbox", kPointer},
        {L"divider", kPointer}, {L"image", kPointer},
        {L"hyperlink", {XmlEventKind::Click, XmlEventKind::DoubleClick,
                        XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        {L"pane", {XmlEventKind::Click, XmlEventKind::DoubleClick,
                    XmlEventKind::MouseDown, XmlEventKind::MouseUp,
                    XmlEventKind::MouseMove}},
        {L"splitview", {XmlEventKind::MouseDown, XmlEventKind::MouseUp,
                       XmlEventKind::MouseMove, XmlEventKind::DoubleClick}},
        {L"scrollview", {XmlEventKind::Scroll, XmlEventKind::Click,
                             XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        {L"flickable", {XmlEventKind::Scroll, XmlEventKind::Click,
                        XmlEventKind::MouseDown, XmlEventKind::MouseUp,
                        XmlEventKind::MouseMove}},
        // 画布
        {L"dwgcanvas", kCanvas}, {L"qcanvas", kCanvas},
        // 浮层/菜单/工具栏
        {L"menu", {XmlEventKind::Click, XmlEventKind::SelectionChanged}},
        {L"menubar", {XmlEventKind::Click, XmlEventKind::SelectionChanged}},
        {L"toolbar", {XmlEventKind::Click, XmlEventKind::SelectionChanged,
                        XmlEventKind::DropDown}},
        {L"popup", {XmlEventKind::Click, XmlEventKind::GotFocus,
                    XmlEventKind::LostFocus}},
        // 日期
        {L"monthgrid", {XmlEventKind::SelectionChanged, XmlEventKind::ValueChanged,
                           XmlEventKind::DoubleClick, XmlEventKind::GotFocus,
                           XmlEventKind::LostFocus}},
        {L"datepicker", {XmlEventKind::SelectionChanged, XmlEventKind::ValueChanged,
                         XmlEventKind::DoubleClick, XmlEventKind::GotFocus,
                         XmlEventKind::LostFocus}},
        // 指示
        // HTML Controls 2 对齐补齐
        {L"frame", kPointer}, {L"dialog", kPointer},
        {L"roundbutton", kPointer}, {L"delaybutton", kCheck}, {L"toolbutton", kPointer},
        {L"toolseparator", kPointer}, {L"tabbutton", kCheck},
        {L"scrollindicator", {XmlEventKind::Scroll, XmlEventKind::Click}},
        {L"menuitem", {XmlEventKind::Click, XmlEventKind::CheckedChanged}},
        {L"menuseparator", kPointer},

        // -------------------------------------------------------------------
        // 补齐：这些键对应各控件构造函数传给 XmlMfcControlBase 的 type()，
        // 之前缺键 ⇒ xmlDefaultEvents 返回空集 ⇒ 该控件声明的所有事件都被
        // 当成"不支持"（典型受害者：XTextField 的 type 是 textbox、XTableView
        // 是 tableview 之外的 gridctrl、XView3D 是 view3d）。
        // 一个 type 只写一次，不在别处再列。
        // -------------------------------------------------------------------
        {L"textbox", kText},          // XmlTextBox  —— XTextField 的本体
        {L"richtext", kText},         // XmlRichText —— XTextArea
        {L"bitmapbutton", kPointer},  // XmlBitmapButton —— XIconButton
        {L"gridctrl", kSelect},       // XmlGrid
        {L"itemview", kSelect},       // XmlItemView
        {L"panel", kPointer},         // XmlPanel
        {L"staticbitmap", kPointer},  // XmlStaticBitmap —— XImage
        {L"staticline", {XmlEventKind::Click, XmlEventKind::DoubleClick}},  // XDivider
        {L"scrolledwindow", {XmlEventKind::Click, XmlEventKind::Scroll,
                             XmlEventKind::GotFocus, XmlEventKind::LostFocus}},
        {L"splitter", {XmlEventKind::Click, XmlEventKind::MouseDown,
                       XmlEventKind::MouseUp, XmlEventKind::MouseMove}},
        {L"view3d", kCanvas},         // XmlView3D
        {L"xribbon", kPointer},       // XmlRibbon

    };
    return kTable;
}

inline const std::vector<XmlEventKind>& xmlDefaultEvents(const std::wstring& type)
{
    static const std::vector<XmlEventKind> kNone;
    const std::map<std::wstring, std::vector<XmlEventKind>>& kTable = xmlEventTable();

    std::wstring t = type;
    for (auto& c : t) c = (wchar_t)towlower(c);
    auto it = kTable.find(t);
    return it == kTable.end() ? kNone : it->second;
}

// 该控件契约是否包含某种事件（type 大小写不敏感）。
inline bool xmlTypeSupportsEvent(const std::wstring& type, XmlEventKind kind)
{
    const std::vector<XmlEventKind>& evs = xmlDefaultEvents(type);
    for (XmlEventKind k : evs)
        if (k == kind)
            return true;
    return false;
}

// 内部键（click / change / …）-> kind（反向查表，唯一实现在这里）
inline bool xmlEventKindOfKey(const std::wstring& key, XmlEventKind& out)
{
    for (XmlEventKind k : {XmlEventKind::Click, XmlEventKind::DoubleClick,
                           XmlEventKind::TextChanged, XmlEventKind::SelectionChanged,
                           XmlEventKind::ValueChanged, XmlEventKind::CheckedChanged,
                           XmlEventKind::DropDown, XmlEventKind::Scroll,
                           XmlEventKind::MouseDown, XmlEventKind::MouseUp,
                           XmlEventKind::MouseMove, XmlEventKind::KeyDown,
                           XmlEventKind::KeyUp, XmlEventKind::GotFocus,
                           XmlEventKind::LostFocus})
    {
        if (key == xmlEventKey(k))
        {
            out = k;
            return true;
        }
    }
    return false;
}

// 兼容旧名（外部若有引用仍可用）
inline std::vector<XmlEventKind> xmlEventsForControl(const std::wstring& type)
{
    return xmlDefaultEvents(type);
}

// ---------------------------------------------------------------------------
// 事件元数据（S2 唯一来源）
//
// XML 属性名、内部键、回调负载，以及"宿主是否已能派发"，只在这里写一次：
//   * 加载器按本表把 XML 属性名解析成 kind 再绑定（不再各处 if 链）；
//   * 设计器按本表出属性面板的事件页；
//   * runtime.py / 外部工具可由 xmlEventSchemaJson() 生成各自的表。
// ---------------------------------------------------------------------------
struct XmlEventInfo
{
    XmlEventKind kind;
    const wchar_t* property;   // XML 写法（Click / TextChanged / …）
    const wchar_t* key;        // 内部键（click / change / …）
    const wchar_t* payload;    // 回调负载说明（无 / text / index / value / checked / vk / x,y,button）
    bool hostDispatch;         // 宿主当前是否已能真派发（false = 加载器直接告警，不静默）
};

inline const std::vector<XmlEventInfo>& xmlEventInfos()
{
    static const std::vector<XmlEventInfo> kInfos = {
        {XmlEventKind::Click,            L"Click",            L"click",      L"",             true},
        {XmlEventKind::DoubleClick,      L"DoubleClick",      L"dblclick",   L"",             true},
        {XmlEventKind::TextChanged,      L"TextChanged",      L"change",     L"text",         true},
        {XmlEventKind::SelectionChanged, L"SelectionChanged", L"select",     L"index",        true},
        {XmlEventKind::ValueChanged,     L"ValueChanged",     L"value",      L"value",        true},
        {XmlEventKind::CheckedChanged,   L"CheckedChanged",   L"checked",    L"checked",      true},
        {XmlEventKind::DropDown,         L"DropDown",         L"dropdown",   L"",             true},
        {XmlEventKind::Scroll,           L"Scroll",           L"scroll",     L"delta",        true},
        {XmlEventKind::MouseDown,        L"MouseDown",        L"mousedown",  L"x,y,button",   true},
        {XmlEventKind::MouseUp,          L"MouseUp",          L"mouseup",    L"x,y,button",   true},
        {XmlEventKind::MouseMove,        L"MouseMove",        L"mousemove",  L"x,y,button",   true},
        {XmlEventKind::KeyDown,          L"KeyDown",          L"keydown",    L"vk",           true},
        {XmlEventKind::KeyUp,            L"KeyUp",            L"keyup",      L"vk",           true},
        {XmlEventKind::GotFocus,         L"GotFocus",         L"gotfocus",   L"",             true},
        {XmlEventKind::LostFocus,        L"LostFocus",        L"lostfocus",  L"",             true},
    };
    return kInfos;
}

// 控件类型 -> 事件键 的 JSON（与上面 events 表同源，设计器/加载器都能查知道自己能挂什么）
inline std::wstring xmlEventTypesJson()
{
    std::wstring out = L"{";
    bool firstType = true;
    for (const auto& kv : xmlEventTable())
    {
        if (!firstType)
            out += L",";
        firstType = false;
        out += L"\"";
        out += kv.first;
        out += L"\":[";
        bool firstKey = true;
        for (XmlEventKind k : kv.second)
        {
            if (!firstKey)
                out += L",";
            firstKey = false;
            out += L"\"";
            out += xmlEventKey(k);
            out += L"\"";
        }
        out += L"]";
    }
    out += L"}";
    return out;
}

// 事件表 -> JSON（ASCII 结构 + 中文说明按 UTF-8 存，供 runtime.py / 设计器生成）
inline std::wstring xmlEventSchemaJson()
{
    std::wstring out = L"{\"version\":1,\"events\":[";
    bool first = true;
    for (const XmlEventInfo& info : xmlEventInfos())
    {
        if (!first)
            out += L",";
        first = false;
        out += L"{\"property\":\"";
        out += info.property;
        out += L"\",\"key\":\"";
        out += info.key;
        out += L"\",\"payload\":\"";
        out += info.payload;
        out += L"\",\"hostDispatch\":";
        out += info.hostDispatch ? L"true" : L"false";
        out += L"}";
    }
    out += L"],\"types\":";
    out += xmlEventTypesJson();
    out += L"}";
    return out;
}

#endif // XML_EVENTS_H
