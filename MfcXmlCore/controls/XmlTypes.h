#pragma once
// XmlTypes —— 控件属性表（S1）。
//
// 目的：一个属性只在一个地方定义一次（名字/别名/类型/枚举值/读写），
// 基类统一按表分派，控件不再"元数据 + setter + getter"手写三份、也不再逐条
// 做 Lower(WideToUtf8(name)) + 字符串线性比较。
//
// 表是类的静态数据（每个控件一份），读写用 std::function 把"具体控件"的
// 成员访问收进来；基类只用 XmlMfcControlBase& 调用，不需要知道子类。
//
// 约定：
//   * Aliases 只放"额外的别名"，主名写在 Name 里，'|' 分隔，大小写不敏感；
//   * Set 为空 = 只读属性（properties() 里 writable=false，设计器置灰）；
//   * Get 为空 = 只写属性（极少见，宁可给个读回值）；
//   * 值统一用宽字符串传递，类型只做语义标注与设计器编辑器选择。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef XML_TYPES_H
#define XML_TYPES_H

#include "XmlCore.h"

#include <functional>
#include <stdio.h>
#include <string>
#include <vector>
#include <cwchar>

// 自绘复合件的种类号（S1/宿主共用）。
// 以前控件里写 addSkinBox(id, 2, ...) 这种字面量、宿主里另有一份 kSkinBoxXxx 常量，
// 枚举一重排就两边错位且编译不报错。现在只有这一份定义，宿主按名字转接。
enum class XmlSkinKind : int
{
    None = 0,
    Switch = 1,      // 开关
    Spin = 2,        // 数字步进
    Range = 3,       // 双滑块区间
    Busy = 4,        // 忙碌指示
    Pages = 5,       // 页码指示
    Dial = 6,        // 圆弧旋钮
    Tumbler = 7,     // 三行滚动选择
    ScrollBar = 8,   // 独立滚动条
    Table = 9,       // 表格
    Tree = 10,       // 树
    ToolBar = 11,
    ToolTip = 12,
    Menu = 13,
    MenuBar = 14,
    Popup = 15,
    ListView = 16,
    Pager = 17,
    GridView = 18,
    PathView = 19,
    QCanvas = 20,
    Flick = 21,
    Shadow = 22,
};

inline constexpr int XmlSkinId(XmlSkinKind k) { return static_cast<int>(k); }

class XmlMfcControlBase;

struct XmlPropEntry
{
    const wchar_t* Name = nullptr;         // 主名（设计器属性面板显示这一列）
    const wchar_t* Aliases = nullptr;      // 额外别名，"Content|Caption"，'|' 分隔
    XmlPropType Type = XmlPropType::String;
    const wchar_t* EnumValues = nullptr;   // Type==Enum 时的候选值 "a|b|c"
    std::function<bool(XmlMfcControlBase&, const std::wstring&)> Set;
    std::function<std::wstring(const XmlMfcControlBase&)> Get;
};

namespace xmlprop2
{
    // 名字/别名匹配：大小写不敏感、整词相等（不做前缀/子串匹配）
    inline bool MatchesName(const wchar_t* name, const XmlPropEntry& entry)
    {
        if (name == nullptr || entry.Name == nullptr)
            return false;
        if (_wcsicmp(name, entry.Name) == 0)
            return true;
        if (entry.Aliases == nullptr)
            return false;
        const wchar_t* p = entry.Aliases;
        while (*p != L'\0')
        {
            const wchar_t* bar = wcschr(p, L'|');
            const size_t len = (bar != nullptr) ? static_cast<size_t>(bar - p) : wcslen(p);
            if (len > 0 && wcslen(name) == len && _wcsnicmp(name, p, len) == 0)
                return true;
            if (bar == nullptr)
                break;
            p = bar + 1;
        }
        return false;
    }

    // ---- 值解析（全库只此一处，替代 _wtoi / xmlExtraNum / propIntOf 三套） ----
    inline std::wstring Trim(const std::wstring& v)
    {
        const size_t b = v.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return {};
        const size_t e = v.find_last_not_of(L" \t\r\n");
        return v.substr(b, e - b + 1);
    }

    inline int ToInt(const std::wstring& v, int fallback)
    {
        const std::wstring t = Trim(v);
        if (t.empty())
            return fallback;
        try
        {
            size_t used = 0;
            const int n = std::stoi(t, &used);
            return (used == 0) ? fallback : n;
        }
        catch (...)
        {
            return fallback;
        }
    }

    inline double ToDouble(const std::wstring& v, double fallback)
    {
        const std::wstring t = Trim(v);
        if (t.empty())
            return fallback;
        try
        {
            size_t used = 0;
            const double n = std::stod(t, &used);
            return (used == 0) ? fallback : n;
        }
        catch (...)
        {
            return fallback;
        }
    }

    inline bool ToBool(const std::wstring& v)
    {
        const std::wstring t = Trim(v);
        if (t.empty())
            return false;
        if (_wcsicmp(t.c_str(), L"0") == 0 || _wcsicmp(t.c_str(), L"false") == 0 ||
            _wcsicmp(t.c_str(), L"no") == 0 || _wcsicmp(t.c_str(), L"off") == 0)
            return false;
        return true;
    }

    inline std::wstring FromBool(bool v) { return v ? L"1" : L"0"; }

    inline std::wstring FromInt(int v) { return std::to_wstring(v); }

    // "a|b|c" -> {a,b,c}，空段丢弃
    inline std::vector<std::wstring> SplitBars(const std::wstring& v, wchar_t sep = L'|')
    {
        std::vector<std::wstring> out;
        size_t start = 0;
        while (start <= v.size())
        {
            const size_t bar = v.find(sep, start);
            const std::wstring seg = (bar == std::wstring::npos) ? v.substr(start)
                                                                 : v.substr(start, bar - start);
            if (!seg.empty())
                out.push_back(seg);
            if (bar == std::wstring::npos)
                break;
            start = bar + 1;
        }
        return out;
    }

    // ---- 未知属性诊断 ----
    // 加载期把"声明了但控件不认"的属性收起来，供设计器/测试查。
    // 默认只入内存表；设 PYARX_PROP_DIAG=1 时同时落 log\xml_prop_diag.log，
    // 避免布局属性（Left/Top/Grid.Row 等由运行时消费）在控制台刷屏。
    inline const std::wstring& LogDir()
    {
        static const std::wstring s_dir = []() -> std::wstring {
            // 允许外部改日志目录；缺省落在系统临时目录，不再写死盘符
            wchar_t env[MAX_PATH] = {};
            if (::GetEnvironmentVariableW(L"PYARX_LOG_DIR", env, MAX_PATH) >= 1)
                return std::wstring(env);
            wchar_t tmp[MAX_PATH] = {};
            ::GetTempPathW(MAX_PATH, tmp);
            return std::wstring(tmp) + L"pyarx";
        }();
        return s_dir;
    }

    inline bool EnvFlagOn(const wchar_t* name)
    {
        wchar_t env[8] = {};
        return ::GetEnvironmentVariableW(name, env, 8) >= 1 && env[0] == L'1';
    }

    // 严格模式：写错的 XML 在加载期直接失败，而不是留一个永远不触发的回调
    inline bool StrictXmlEnabled()
    {
        static const bool s_strict = EnvFlagOn(L"PYARX_STRICT_XML");
        return s_strict;
    }

    struct PropWarning
    {
        std::wstring Control;
        std::wstring Property;
    };

    inline std::vector<PropWarning>& WarningSink()
    {
        static std::vector<PropWarning> s_warnings;
        return s_warnings;
    }

    inline void WarnUnknown(const std::wstring& control, const std::wstring& property)
    {
        static const bool s_logToFile = []() {
            wchar_t env[8] = {};
            return ::GetEnvironmentVariableW(L"PYARX_PROP_DIAG", env, 8) >= 1 && env[0] == L'1';
        }();
        // 同 (控件,属性) 只记一次
        for (const PropWarning& w : WarningSink())
        {
            if (w.Control == control && w.Property == property)
                return;
        }
        WarningSink().push_back({control, property});
        if (s_logToFile)
        {
            FILE* f = nullptr;
            std::wstring path = LogDir() + L"\\xml_prop_diag.log";
            _wfopen_s(&f, path.c_str(), L"a");
            if (f != nullptr)
            {
                std::fprintf(f, "%ls.%ls\n", control.c_str(), property.c_str());
                std::fclose(f);
            }
        }
    }

    inline const std::vector<PropWarning>& Warnings() { return WarningSink(); }

    inline void ClearWarnings() { WarningSink().clear(); }

    // -----------------------------------------------------------------------
    // 事件契约诊断：控件声明了它自己不会产生的事件（死绑定）。
    // 以前这类写法静默失效，现在进诊断表，PYARX_EVENT_DIAG=1 时落文件。
    // -----------------------------------------------------------------------
    struct EventWarning
    {
        std::wstring Control;
        std::wstring Event;
        std::wstring Callback;
    };

    inline std::vector<EventWarning>& EventWarningSink()
    {
        static std::vector<EventWarning> s_events;
        return s_events;
    }

    inline void WarnUnsupportedEvent(const std::wstring& control, const std::wstring& event,
                                     const std::wstring& callback)
    {
        // 同 (控件,事件,回调) 只记一次
        for (const EventWarning& w : EventWarningSink())
        {
            if (w.Control == control && w.Event == event && w.Callback == callback)
                return;
        }
        EventWarningSink().push_back({control, event, callback});
        if (EnvFlagOn(L"PYARX_EVENT_DIAG"))
        {
            std::wstring path = LogDir() + L"\\xml_event_diag.log";
            FILE* f = nullptr;
            _wfopen_s(&f, path.c_str(), L"a");
            if (f != nullptr)
            {
                std::fprintf(f, "%ls.%ls -> %ls  (控件不会产生该事件)\n",
                             control.c_str(), event.c_str(), callback.c_str());
                std::fclose(f);
            }
        }
    }

    inline const std::vector<EventWarning>& EventWarnings() { return EventWarningSink(); }

    inline void ClearEventWarnings() { EventWarningSink().clear(); }
}  // namespace xmlprop2

#endif // XML_TYPES_H
