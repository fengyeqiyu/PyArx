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
inline const std::vector<XmlEventKind>& xmlDefaultEvents(const std::wstring& type)
{
    static const std::vector<XmlEventKind> kNone;

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
        {L"textfield", kText}, {L"textarea", kText}, {L"codeeditor", kText},
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
        {L"tabbar", kFairly}, {L"tableview", kSelect}, {L"treetable", kSelect},
        {L"gridlayout", kSelect}, {L"gridview", kSelect}, {L"pathview", kSelect},
        {L"virtuallist", kSelect}, {L"stackview", kFairly}, {L"swipeview", kFairly},
        {L"page", kFairly}, {L"pageindicator", kFairly},
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
        {L"powercanvas", kCanvas}, {L"qcanvas", kCanvas},
        // 浮层/菜单/工具栏
        {L"menu", {XmlEventKind::Click, XmlEventKind::SelectionChanged}},
        {L"menubar", {XmlEventKind::Click, XmlEventKind::SelectionChanged}},
        {L"toolbar", {XmlEventKind::Click, XmlEventKind::SelectionChanged,
                        XmlEventKind::DropDown}},
        {L"tooltip", {XmlEventKind::Click}},
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
        {L"busyindicator", {XmlEventKind::Click, XmlEventKind::GotFocus,
                            XmlEventKind::LostFocus}},
        // QML Controls 2 对齐补齐
        {L"frame", kPointer}, {L"dialog", kPointer}, {L"drawer", kPointer},
        {L"roundbutton", kPointer}, {L"delaybutton", kCheck}, {L"toolbutton", kPointer},
        {L"toolseparator", kPointer}, {L"tabbutton", kCheck},
        {L"scrollindicator", {XmlEventKind::Scroll, XmlEventKind::Click}},
        {L"menuitem", {XmlEventKind::Click, XmlEventKind::CheckedChanged}},
        {L"menuseparator", kPointer},

    };

    std::wstring t = type;
    for (auto& c : t) c = (wchar_t)towlower(c);
    auto it = kTable.find(t);
    return it == kTable.end() ? kNone : it->second;
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
    out += L"]}";
    return out;
}

#endif // XML_EVENTS_H
