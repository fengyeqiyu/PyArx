#pragma once
// XamlEvents —— 控件事件契约（唯一来源）。
//
// 前端（XAML）只声明事件名：<Button Click="onOk" />、<ComboBox SelectionChanged="onSel" />；
// 后端（控件实现 + 宿主）负责把原生消息派发成事件并调用回调。
// 回调缺省时事件静默丢弃，绝不报错——事件机制本身必须齐全。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XAML_EVENTS_H
#define XAML_EVENTS_H

#include <string>
#include <vector>
#include <map>
#include <cwctype>
#include <cmath>

enum class XamlEventKind
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
inline const wchar_t* xamlEventKey(XamlEventKind k)
{
    switch (k)
    {
    case XamlEventKind::Click: return L"click";
    case XamlEventKind::DoubleClick: return L"dblclick";
    case XamlEventKind::TextChanged: return L"change";
    case XamlEventKind::SelectionChanged: return L"select";
    case XamlEventKind::ValueChanged: return L"value";
    case XamlEventKind::CheckedChanged: return L"checked";
    case XamlEventKind::DropDown: return L"dropdown";
    case XamlEventKind::Scroll: return L"scroll";
    case XamlEventKind::MouseDown: return L"mousedown";
    case XamlEventKind::MouseUp: return L"mouseup";
    case XamlEventKind::MouseMove: return L"mousemove";
    case XamlEventKind::KeyDown: return L"keydown";
    case XamlEventKind::KeyUp: return L"keyup";
    case XamlEventKind::GotFocus: return L"gotfocus";
    case XamlEventKind::LostFocus: return L"lostfocus";
    }
    return L"";
}

// XAML 属性名（前端写法）
inline const wchar_t* xamlEventProperty(XamlEventKind k)
{
    switch (k)
    {
    case XamlEventKind::Click: return L"Click";
    case XamlEventKind::DoubleClick: return L"DoubleClick";
    case XamlEventKind::TextChanged: return L"TextChanged";
    case XamlEventKind::SelectionChanged: return L"SelectionChanged";
    case XamlEventKind::ValueChanged: return L"ValueChanged";
    case XamlEventKind::CheckedChanged: return L"CheckedChanged";
    case XamlEventKind::DropDown: return L"DropDown";
    case XamlEventKind::Scroll: return L"Scroll";
    case XamlEventKind::MouseDown: return L"MouseDown";
    case XamlEventKind::MouseUp: return L"MouseUp";
    case XamlEventKind::MouseMove: return L"MouseMove";
    case XamlEventKind::KeyDown: return L"KeyDown";
    case XamlEventKind::KeyUp: return L"KeyUp";
    case XamlEventKind::GotFocus: return L"GotFocus";
    case XamlEventKind::LostFocus: return L"LostFocus";
    }
    return L"";
}

// 属性名 -> kind（不区分大小写）。不是事件属性返回 false。
inline bool xamlEventFromProperty(const std::wstring& name, XamlEventKind& out)
{
    std::wstring t = name;
    for (auto& c : t) c = (wchar_t)towlower(c);
    if (t == L"click" || t == L"onclick") { out = XamlEventKind::Click; return true; }
    if (t == L"doubleclick" || t == L"dblclick" || t == L"ondoubleclick") { out = XamlEventKind::DoubleClick; return true; }
    if (t == L"textchanged" || t == L"onchange") { out = XamlEventKind::TextChanged; return true; }
    if (t == L"selectionchanged" || t == L"onselect") { out = XamlEventKind::SelectionChanged; return true; }
    if (t == L"valuechanged" || t == L"onvalue") { out = XamlEventKind::ValueChanged; return true; }
    if (t == L"checkedchanged" || t == L"onchecked") { out = XamlEventKind::CheckedChanged; return true; }
    if (t == L"dropdown" || t == L"ondropdown") { out = XamlEventKind::DropDown; return true; }
    if (t == L"scroll" || t == L"onscroll") { out = XamlEventKind::Scroll; return true; }
    if (t == L"mousedown" || t == L"onmousedown") { out = XamlEventKind::MouseDown; return true; }
    if (t == L"mouseup" || t == L"onmouseup") { out = XamlEventKind::MouseUp; return true; }
    if (t == L"mousemove" || t == L"onmousemove") { out = XamlEventKind::MouseMove; return true; }
    if (t == L"keydown" || t == L"onkeydown") { out = XamlEventKind::KeyDown; return true; }
    if (t == L"keyup" || t == L"onkeyup") { out = XamlEventKind::KeyUp; return true; }
    if (t == L"gotfocus" || t == L"onfocus") { out = XamlEventKind::GotFocus; return true; }
    if (t == L"lostfocus" || t == L"onblur") { out = XamlEventKind::LostFocus; return true; }
    return false;
}

// 每种控件"该有的事件"（S2）：由控件类型名给出缺省集合。
// 数据驱动（一张表），不再用 if 链逐个类型名分支；控件类可用 events() 覆盖。
//
// 类型名用小写短名，与各 XamlXxx 构造函数里传给 XamlMfcControlBase 的一致。
inline const std::vector<XamlEventKind>& xamlDefaultEvents(const std::wstring& type)
{
    static const std::vector<XamlEventKind> kNone;

    static const std::vector<XamlEventKind> kPointer = {
        XamlEventKind::Click, XamlEventKind::DoubleClick, XamlEventKind::MouseDown,
        XamlEventKind::MouseUp, XamlEventKind::MouseMove, XamlEventKind::KeyDown,
        XamlEventKind::KeyUp, XamlEventKind::GotFocus, XamlEventKind::LostFocus
    };
    static const std::vector<XamlEventKind> kText = {
        XamlEventKind::TextChanged, XamlEventKind::KeyDown, XamlEventKind::KeyUp,
        XamlEventKind::GotFocus, XamlEventKind::LostFocus, XamlEventKind::Click,
        XamlEventKind::DoubleClick
    };
    static const std::vector<XamlEventKind> kSelect = {
        XamlEventKind::SelectionChanged, XamlEventKind::Click, XamlEventKind::DoubleClick,
        XamlEventKind::Scroll, XamlEventKind::GotFocus, XamlEventKind::LostFocus
    };
    static const std::vector<XamlEventKind> kValue = {
        XamlEventKind::ValueChanged, XamlEventKind::Scroll, XamlEventKind::GotFocus,
        XamlEventKind::LostFocus, XamlEventKind::Click
    };
    static const std::vector<XamlEventKind> kCheck = {
        XamlEventKind::Click, XamlEventKind::CheckedChanged, XamlEventKind::DoubleClick,
        XamlEventKind::GotFocus, XamlEventKind::LostFocus
    };
    static const std::vector<XamlEventKind> kFairly = {
        XamlEventKind::Click, XamlEventKind::SelectionChanged, XamlEventKind::GotFocus,
        XamlEventKind::LostFocus
    };
    static const std::vector<XamlEventKind> kCanvas = {
        XamlEventKind::Click, XamlEventKind::DoubleClick, XamlEventKind::MouseDown,
        XamlEventKind::MouseUp, XamlEventKind::MouseMove, XamlEventKind::Scroll,
        XamlEventKind::KeyDown, XamlEventKind::KeyUp, XamlEventKind::GotFocus,
        XamlEventKind::LostFocus
    };

    // 键 = 控件契约名（= 注册标签的小写形式），一个控件一个名字，不写别名
    static const std::map<std::wstring, std::vector<XamlEventKind>> kTable = {
        // 按钮族
        {L"button", kPointer}, {L"togglebutton", kCheck}, {L"bitmapbutton", kPointer},
        {L"checkbox", kCheck},
        {L"radiobutton", {XamlEventKind::Click, XamlEventKind::CheckedChanged,
                          XamlEventKind::GotFocus, XamlEventKind::LostFocus}},
        // 文本族
        {L"textbox", kText}, {L"richtext", kText}, {L"codeeditor", kText},
        // 选择族
        {L"combobox", {XamlEventKind::SelectionChanged, XamlEventKind::TextChanged,
                       XamlEventKind::DropDown, XamlEventKind::DoubleClick,
                       XamlEventKind::KeyDown, XamlEventKind::KeyUp,
                       XamlEventKind::GotFocus, XamlEventKind::LostFocus}},
        {L"choice", {XamlEventKind::SelectionChanged, XamlEventKind::DropDown,
                     XamlEventKind::DoubleClick, XamlEventKind::KeyDown,
                     XamlEventKind::KeyUp, XamlEventKind::GotFocus,
                     XamlEventKind::LostFocus}},
        {L"listbox", kSelect}, {L"listctrl", kSelect}, {L"treectrl", kSelect},
        {L"tabcontrol", kFairly}, {L"tableview", kSelect}, {L"treetable", kSelect},
        {L"gridctrl", kSelect}, {L"gridview", kSelect}, {L"pathview", kSelect},
        {L"virtuallist", kSelect}, {L"stackview", kFairly}, {L"swipeview", kFairly},
        {L"page", kFairly}, {L"pageindicator", kFairly},
        // 取值族
        {L"slider", kValue}, {L"rangeslider", kValue}, {L"progressbar", kValue},
        {L"dial", kValue}, {L"tumbler", kValue}, {L"spinbox", kValue},
        {L"scrollbar", {XamlEventKind::ValueChanged, XamlEventKind::Scroll,
                        XamlEventKind::GotFocus, XamlEventKind::LostFocus}},
        {L"switch", {XamlEventKind::Click, XamlEventKind::CheckedChanged,
                     XamlEventKind::ValueChanged, XamlEventKind::GotFocus,
                     XamlEventKind::LostFocus}},
        // 静态/容器
        {L"textblock", kPointer}, {L"groupbox", kPointer},
        {L"staticline", kPointer}, {L"staticbitmap", kPointer},
        {L"hyperlink", {XamlEventKind::Click, XamlEventKind::DoubleClick,
                        XamlEventKind::GotFocus, XamlEventKind::LostFocus}},
        {L"panel", {XamlEventKind::Click, XamlEventKind::DoubleClick,
                    XamlEventKind::MouseDown, XamlEventKind::MouseUp,
                    XamlEventKind::MouseMove}},
        {L"splitter", {XamlEventKind::MouseDown, XamlEventKind::MouseUp,
                       XamlEventKind::MouseMove, XamlEventKind::DoubleClick}},
        {L"scrolledwindow", {XamlEventKind::Scroll, XamlEventKind::Click,
                             XamlEventKind::GotFocus, XamlEventKind::LostFocus}},
        {L"flickable", {XamlEventKind::Scroll, XamlEventKind::Click,
                        XamlEventKind::MouseDown, XamlEventKind::MouseUp,
                        XamlEventKind::MouseMove}},
        // 画布
        {L"powercanvas", kCanvas}, {L"qcanvas", kCanvas},
        // 浮层/菜单/工具栏
        {L"menu", {XamlEventKind::Click, XamlEventKind::SelectionChanged}},
        {L"menustrip", {XamlEventKind::Click, XamlEventKind::SelectionChanged}},
        {L"toolstrip", {XamlEventKind::Click, XamlEventKind::SelectionChanged,
                        XamlEventKind::DropDown}},
        {L"tooltip", {XamlEventKind::Click}},
        {L"popup", {XamlEventKind::Click, XamlEventKind::GotFocus,
                    XamlEventKind::LostFocus}},
        // 日期
        {L"calendarctrl", {XamlEventKind::SelectionChanged, XamlEventKind::ValueChanged,
                           XamlEventKind::DoubleClick, XamlEventKind::GotFocus,
                           XamlEventKind::LostFocus}},
        {L"datepicker", {XamlEventKind::SelectionChanged, XamlEventKind::ValueChanged,
                         XamlEventKind::DoubleClick, XamlEventKind::GotFocus,
                         XamlEventKind::LostFocus}},
        // 指示
        {L"busyindicator", {XamlEventKind::Click, XamlEventKind::GotFocus,
                            XamlEventKind::LostFocus}},
    };

    std::wstring t = type;
    for (auto& c : t) c = (wchar_t)towlower(c);
    auto it = kTable.find(t);
    return it == kTable.end() ? kNone : it->second;
}

// 兼容旧名（外部若有引用仍可用）
inline std::vector<XamlEventKind> xamlEventsForControl(const std::wstring& type)
{
    return xamlDefaultEvents(type);
}

// ---------------------------------------------------------------------------
// 事件元数据（S2 唯一来源）
//
// XAML 属性名、内部键、回调负载，以及"宿主是否已能派发"，只在这里写一次：
//   * 加载器按本表把 XAML 属性名解析成 kind 再绑定（不再各处 if 链）；
//   * 设计器按本表出属性面板的事件页；
//   * runtime.py / 外部工具可由 xamlEventSchemaJson() 生成各自的表。
// ---------------------------------------------------------------------------
struct XamlEventInfo
{
    XamlEventKind kind;
    const wchar_t* property;   // XAML 写法（Click / TextChanged / …）
    const wchar_t* key;        // 内部键（click / change / …）
    const wchar_t* payload;    // 回调负载说明（无 / text / index / value / checked / vk / x,y,button）
    bool hostDispatch;         // 宿主当前是否已能真派发（false = 加载器直接告警，不静默）
};

inline const std::vector<XamlEventInfo>& xamlEventInfos()
{
    static const std::vector<XamlEventInfo> kInfos = {
        {XamlEventKind::Click,            L"Click",            L"click",      L"",             true},
        {XamlEventKind::DoubleClick,      L"DoubleClick",      L"dblclick",   L"",             true},
        {XamlEventKind::TextChanged,      L"TextChanged",      L"change",     L"text",         true},
        {XamlEventKind::SelectionChanged, L"SelectionChanged", L"select",     L"index",        true},
        {XamlEventKind::ValueChanged,     L"ValueChanged",     L"value",      L"value",        true},
        {XamlEventKind::CheckedChanged,   L"CheckedChanged",   L"checked",    L"checked",      true},
        {XamlEventKind::DropDown,         L"DropDown",         L"dropdown",   L"",             true},
        {XamlEventKind::Scroll,           L"Scroll",           L"scroll",     L"delta",        true},
        {XamlEventKind::MouseDown,        L"MouseDown",        L"mousedown",  L"x,y,button",   true},
        {XamlEventKind::MouseUp,          L"MouseUp",          L"mouseup",    L"x,y,button",   true},
        {XamlEventKind::MouseMove,        L"MouseMove",        L"mousemove",  L"x,y,button",   true},
        {XamlEventKind::KeyDown,          L"KeyDown",          L"keydown",    L"vk",           true},
        {XamlEventKind::KeyUp,            L"KeyUp",            L"keyup",      L"vk",           true},
        {XamlEventKind::GotFocus,         L"GotFocus",         L"gotfocus",   L"",             true},
        {XamlEventKind::LostFocus,        L"LostFocus",        L"lostfocus",  L"",             true},
    };
    return kInfos;
}

// 事件表 -> JSON（ASCII 结构 + 中文说明按 UTF-8 存，供 runtime.py / 设计器生成）
inline std::wstring xamlEventSchemaJson()
{
    std::wstring out = L"{\"version\":1,\"events\":[";
    bool first = true;
    for (const XamlEventInfo& info : xamlEventInfos())
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

#endif // XAML_EVENTS_H
