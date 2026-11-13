#include "XamlMfcControlBase.h"

#include <windows.h>

namespace
{
    // 属性值 -> int：解析失败时退回调用方给的默认值
    int propIntOf(const std::wstring& value, int fallback)
    {
        std::wstring text = value;
        const size_t b = text.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return fallback;
        const size_t e = text.find_last_not_of(L" \t\r\n");
        text = (e == std::wstring::npos) ? text.substr(b) : text.substr(b, e - b + 1);
        if (text.empty())
            return fallback;
        try
        {
            return std::stoi(text);
        }
        catch (...)
        {
            return fallback;
        }
    }

    // XAML 事件属性名 -> 统一 kind（小写）
    // 全部来自 XamlEvents 的契约表，不在本文件另立一份，避免两处漂移。
    std::wstring EventKind(const std::wstring& name)
    {
        XamlEventKind k = XamlEventKind::Click;
        if (xamlEventFromProperty(name, k))
            return xamlEventKey(k);
        return {};
    }

    // kind -> 对外展示的事件属性名（用于 getProperty 回读）
    std::wstring EventProperty(const std::wstring& kind)
    {
        static const XamlEventKind kAll[] = {
            XamlEventKind::Click, XamlEventKind::DoubleClick, XamlEventKind::TextChanged,
            XamlEventKind::SelectionChanged, XamlEventKind::ValueChanged, XamlEventKind::CheckedChanged,
            XamlEventKind::DropDown, XamlEventKind::Scroll, XamlEventKind::MouseDown,
            XamlEventKind::MouseUp, XamlEventKind::MouseMove, XamlEventKind::KeyDown,
            XamlEventKind::KeyUp, XamlEventKind::GotFocus, XamlEventKind::LostFocus
        };
        for (XamlEventKind k : kAll)
            if (kind == xamlEventKey(k))
                return xamlEventProperty(k);
        return L"";
    }
}  // namespace

const wchar_t* XamlMfcControlBase::type() const
{
    return m_type.c_str();
}

XamlMfcControlBase::XamlMfcControlBase(std::wstring type)
{
    m_type = std::move(type);
}

void XamlMfcControlBase::attach(IXamlMfcHost* host, int id, int minValue, int maxValue,
                                int x, int y, int w, int h)
{
    m_host = host;
    m_id = id;
    m_min = minValue;
    m_max = maxValue;
    if (host == nullptr)
        return;
    createNative(host, id, minValue, maxValue, x, y, w, h);
    pushAppearance();
}

void XamlMfcControlBase::pushAppearance()
{
    // 先让子类把外观字段同步到自己的内部格式（无宿主的纯声明阶段也要生效）
    onAppearanceChanged();
    if (m_host != nullptr && m_id >= 1000 && m_id <= 65000 && !m_ap.empty())
        m_host->setControlAppearance(m_id, m_ap);
}

void XamlMfcControlBase::setAppearance(const xamlapp::XamlAppearance& ap)
{
    m_ap = ap;
    pushAppearance();
}

void XamlMfcControlBase::setLevel(int v)
{
    m_level = v;
    pushOrder();
}

void XamlMfcControlBase::setTabIndex(int v)
{
    m_tabIndex = v;
    pushOrder();
}

void XamlMfcControlBase::setTheme(const std::wstring& v)
{
    m_theme = v;
    pushTheme();
}

void XamlMfcControlBase::pushTheme()
{
    if (m_host == nullptr || m_id < 1000 || m_id > 65000)
        return;
    m_host->setControlTheme(m_id, m_theme);
}

void XamlMfcControlBase::pushOrder()
{
    if (m_host == nullptr || m_id < 1000 || m_id > 65000)
        return;
    m_host->setControlLevel(m_id, m_level);
    m_host->setControlTabIndex(m_id, m_tabIndex);
}

bool XamlMfcControlBase::create(void* /*parentHandle*/)
{
    // 标准控件由宿主在回调中 attach 完成创建；此处仅为契约兼容
    m_created = true;
    return true;
}

void XamlMfcControlBase::destroy()
{
    if (m_host != nullptr && m_id >= 1000 && m_id <= 65000)
    {
        if (HWND h = static_cast<HWND>(m_host->getDlgItemHandle(m_id)))
            ::DestroyWindow(h);
    }
    m_created = false;
}

void* XamlMfcControlBase::nativeHandle() const
{
    if (m_host == nullptr)
        return nullptr;
    return m_host->getDlgItemHandle(m_id);
}

void XamlMfcControlBase::setBounds(int x, int y, int w, int h)
{
    if (m_host != nullptr)
        m_host->moveControl(m_id, x, y, w, h);
}

void XamlMfcControlBase::getBounds(int& x, int& y, int& w, int& h) const
{
    x = y = w = h = 0;
    HWND hwnd = static_cast<HWND>(nativeHandle());
    if (hwnd == nullptr)
        return;
    RECT rc = {};
    ::GetWindowRect(hwnd, &rc);
    HWND parent = ::GetParent(hwnd);
    if (parent != nullptr)
        ::ScreenToClient(parent, reinterpret_cast<POINT*>(&rc.left));
    x = rc.left;
    y = rc.top;
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
}

std::vector<XamlProperty> XamlMfcControlBase::properties() const
{
    // 通用层次属性排在自有属性之前：属性面板里先看到"所有控件都一样"的部分
    static const XamlProperty kOrderProps[] = {
        {L"Level", XamlPropType::Int, true, true, nullptr},
        {L"TabIndex", XamlPropType::Int, true, true, nullptr},
        {L"Theme", XamlPropType::String, true, true, nullptr},
    };
    std::vector<XamlProperty> out = xamlapp::Properties();
    for (const XamlProperty& p : kOrderProps)
        out.push_back(p);

    // 属性表（S1）：一个属性只在这里出现一次；Set 为空 = 只读
    size_t tableCount = 0;
    const XamlPropEntry* table = ownPropTable(tableCount);
    for (size_t i = 0; i < tableCount; ++i)
    {
        const XamlPropEntry& e = table[i];
        if (e.Name == nullptr)
            continue;
        const bool writable = static_cast<bool>(e.Set);
        // XamlProperty 字段序是 name / type / readable / writable / enumValues
        out.push_back({e.Name, e.Type, true, writable, e.EnumValues});
    }

    for (const auto& p : ownProperties())
        out.push_back(p);
    return out;
}

bool XamlMfcControlBase::setProperty(const wchar_t* name, const std::wstring& value)
{
    if (name == nullptr)
        return false;

    // 0) QML 写法归一：text / color / font.pixelSize / currentIndex …
    const std::wstring key = xamlmfc::QmlPropName(name);

    // 1) 统一外观：任何控件都认
    if (xamlapp::SetProperty(m_ap, key, value))
    {
        pushAppearance();
        return true;
    }

    // 2) 所有控件共有的层次属性：Level / ZIndex（叠放），TabIndex（键盘序）
    if (_wcsicmp(key.c_str(), L"level") == 0 || _wcsicmp(key.c_str(), L"zindex") == 0 ||
        _wcsicmp(key.c_str(), L"panel.zindex") == 0)
    {
        setLevel(propIntOf(value, 0));
        return true;
    }
    if (_wcsicmp(key.c_str(), L"tabindex") == 0)
    {
        setTabIndex(propIntOf(value, -1));
        return true;
    }
    if (_wcsicmp(key.c_str(), L"theme") == 0)
    {
        setTheme(value);
        return true;
    }

    // 3) 事件声明：Click="onFoo" 等
    const std::wstring kind = EventKind(key);
    if (!kind.empty())
    {
        for (auto& kv : m_eventNames)
        {
            if (kv.first == kind)
            {
                kv.second = value;
                return true;
            }
        }
        m_eventNames.emplace_back(kind, value);
        return true;
    }

    // 4) 属性表（S1）
    bool handled = false;
    if (setFromPropTable(name, value, handled))
        return true;
    if (handled)
        return false;   // 命中但只读/暂不可写：明确拒绝，不再往下猜

    // 5) 旧路径（未迁移控件）
    if (setOwnProperty(name, value))
        return true;

    // 6) 真正不认识的属性：入诊断表（PYARX_PROP_DIAG=1 时同时落文件），不再静默
    if (!IsLayoutAttribute(key))
        xamlprop2::WarnUnknown(m_type, key);
    return false;
}

std::wstring XamlMfcControlBase::getProperty(const wchar_t* name) const
{
    if (name == nullptr)
        return {};
    const std::wstring key = xamlmfc::QmlPropName(name);

    std::wstring ap = xamlapp::GetProperty(m_ap, key);
    if (!ap.empty())
        return ap;
    if (xamlapp::IsAppearanceProperty(key))
        return ap;   // 外观属性已声明为空值

    if (_wcsicmp(key.c_str(), L"level") == 0 || _wcsicmp(key.c_str(), L"zindex") == 0 ||
        _wcsicmp(key.c_str(), L"panel.zindex") == 0)
        return std::to_wstring(m_level);
    if (_wcsicmp(key.c_str(), L"tabindex") == 0)
        return std::to_wstring(m_tabIndex);
    if (_wcsicmp(key.c_str(), L"theme") == 0)
        return m_theme;

    const std::wstring kind = EventKind(key);
    if (!kind.empty())
        return eventName(kind.c_str());

    // 属性表（S1）优先，旧路径兜底
    bool handled = false;
    const std::wstring tableValue = getFromPropTable(name, handled);
    if (handled)
        return tableValue;

    return getOwnProperty(name);
}

// 表驱动读写：命中（名字或别名）即 handled=true；
// Set 为空表示只读（写返回 false 但不落旧路径），Get 为空返回空串。
bool XamlMfcControlBase::setFromPropTable(const wchar_t* name, const std::wstring& value, bool& handled)
{
    size_t count = 0;
    const XamlPropEntry* table = ownPropTable(count);
    if (table == nullptr || name == nullptr)
        return false;
    for (size_t i = 0; i < count; ++i)
    {
        if (!xamlprop2::MatchesName(name, table[i]))
            continue;
        handled = true;
        if (!table[i].Set)
            return false;
        return table[i].Set(*this, value);
    }
    return false;
}

std::wstring XamlMfcControlBase::getFromPropTable(const wchar_t* name, bool& handled) const
{
    size_t count = 0;
    const XamlPropEntry* table = ownPropTable(count);
    if (table == nullptr || name == nullptr)
        return {};
    for (size_t i = 0; i < count; ++i)
    {
        if (!xamlprop2::MatchesName(name, table[i]))
            continue;
        handled = true;
        if (!table[i].Get)
            return {};
        return table[i].Get(*this);
    }
    return {};
}

// 布局/结构类属性由运行时与宿主消费，控件本身不认识属正常，不进诊断表
bool XamlMfcControlBase::IsLayoutAttribute(const std::wstring& key)
{
    static const wchar_t* kLayout[] = {
        L"left", L"top", L"right", L"bottom", L"x", L"y", L"w", L"h",
        L"width", L"height", L"minwidth", L"minheight", L"maxwidth", L"maxheight",
        L"margin", L"padding", L"dock", L"row", L"column", L"rowspan", L"columnspan",
        L"align", L"halign", L"valign", L"visible", L"enabled", L"opacity",
        L"style", L"template", L"tag", L"tooltip", L"cursor", L"zindex",
        // QML 布局写法（对齐 QML 的 Layout 附加属性与 anchors）
        L"layout.row", L"layout.column", L"layout.rowspan", L"layout.columnspan",
        L"layout.preferredwidth", L"layout.preferredheight",
        L"layout.fillwidth", L"layout.fillheight", L"layout.alignment",
        L"layout.margins", L"layout.minimumwidth", L"layout.minimumheight",
        L"anchors.fill", L"anchors.left", L"anchors.right", L"anchors.top",
        L"anchors.bottom", L"anchors.centering", L"anchors.centerin",
        L"anchors.margins", L"anchors.horizontalcenter", L"anchors.verticalcenter",
        L"name", L"x:name"    // 节点名由加载器消费，控件不认识属正常
    };
    if (key.find(L'.') != std::wstring::npos)   // Grid.Row / Panel.ZIndex 等附加属性
        return true;
    for (const wchar_t* k : kLayout)
    {
        if (_wcsicmp(key.c_str(), k) == 0)
            return true;
    }
    return false;
}

bool XamlMfcControlBase::bindEvent(const wchar_t* kind, void* callback)
{
    const std::wstring k = kind != nullptr ? kind : L"";
    for (auto& kv : m_handlers)
    {
        if (kv.first == k)
        {
            kv.second = callback;
            return true;
        }
    }
    m_handlers.emplace_back(k, callback);
    return true;
}

void* XamlMfcControlBase::eventHandler(const wchar_t* kind) const
{
    const std::wstring k = kind != nullptr ? kind : L"";
    for (const auto& kv : m_handlers)
        if (kv.first == k)
            return kv.second;
    return nullptr;
}

std::wstring XamlMfcControlBase::eventName(const wchar_t* kind) const
{
    const std::wstring k = kind != nullptr ? kind : L"";
    for (const auto& kv : m_eventNames)
        if (kv.first == k)
            return kv.second;
    return {};
}

void XamlMfcControlBase::clearHandlers()
{
    m_handlers.clear();
}
