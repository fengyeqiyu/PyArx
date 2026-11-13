// XamlComboBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlComboBox.h"

void XamlComboBox::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
    host->addComboAdvanced(id, m_dropDownList, x, y, w, h);
    if (!m_itemsCache.empty())
        fillItems();
    if (m_selectedIndex >= 0)
        host->setCurSel(id, m_selectedIndex);
    else if (!m_text.empty())
        host->setText(id, m_text);
}

// 属性表：Items / Text / SelectedIndex / DropDownStyle 各一行，别名归一。
const std::vector<XamlPropEntry>& XamlComboBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlComboBox&>(b).SetItemsProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlComboBox&>(b).GetItemsProp(); }},
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlComboBox&>(b).SetTextProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlComboBox&>(b).GetTextProp(); }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlComboBox&>(b).SetSelectedIndexProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlComboBox&>(b).GetSelectedIndexProp(); }},
        {L"DropDownStyle", nullptr, XamlPropType::Enum, L"DropDownList|DropDown",
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlComboBox&>(b).SetDropDownStyleProp(v); },
         [](const XamlMfcControlBase& b) {
             return static_cast<const XamlComboBox&>(b).m_dropDownList ? L"DropDownList" : L"DropDown"; }},
    };
    return kTable;
}

bool XamlComboBox::SetItemsProp(const std::wstring& v)
{
    m_itemsCache = v;
    fillItems();
    return true;
}

std::wstring XamlComboBox::GetItemsProp() const
{
    if (m_host == nullptr)
        return m_itemsCache;
    std::wstring out;
    const int n = m_host->countStrings(m_id);
    for (int i = 0; i < n; ++i)
    {
        if (i > 0) out += L"|";
        out += m_host->getString(m_id, i);
    }
    return out;
}

bool XamlComboBox::SetTextProp(const std::wstring& v)
{
    m_text = v;
    if (m_host != nullptr)
        m_host->setText(m_id, v);
    return true;
}

std::wstring XamlComboBox::GetTextProp() const
{
    return m_host != nullptr ? m_host->getText(m_id) : m_text;
}

bool XamlComboBox::SetSelectedIndexProp(const std::wstring& v)
{
    m_selectedIndex = xamlprop2::ToInt(v, -1);
    if (m_host != nullptr)
        m_host->setCurSel(m_id, m_selectedIndex);
    return true;
}

std::wstring XamlComboBox::GetSelectedIndexProp() const
{
    if (m_host == nullptr)
        return xamlprop2::FromInt(m_selectedIndex);
    return xamlprop2::FromInt(m_host->getCurSel(m_id));
}

bool XamlComboBox::SetDropDownStyleProp(const std::wstring& v)
{
    // DropDownList：只能选；DropDown：可编辑（也能手打文字）
    const std::wstring t = xamlprop2::Trim(v);
    const bool listOnly = (_wcsicmp(t.c_str(), L"DropDownList") == 0 ||
                           _wcsicmp(t.c_str(), L"list") == 0);
    if (listOnly != m_dropDownList)
    {
        m_dropDownList = listOnly;
        rebuild();
    }
    return true;
}
