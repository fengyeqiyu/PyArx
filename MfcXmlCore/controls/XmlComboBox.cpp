// XmlComboBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XmlComboBox.h"

void XmlComboBox::createNative(IXmlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
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
const std::vector<XmlPropEntry>& XmlComboBox::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlComboBox&>(b).SetItemsProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlComboBox&>(b).GetItemsProp(); }},
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlComboBox&>(b).SetTextProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlComboBox&>(b).GetTextProp(); }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlComboBox&>(b).SetSelectedIndexProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlComboBox&>(b).GetSelectedIndexProp(); }},
        {L"DropDownStyle", nullptr, XmlPropType::Enum, L"DropDownList|DropDown",
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlComboBox&>(b).SetDropDownStyleProp(v); },
         [](const XmlMfcControlBase& b) {
             return static_cast<const XmlComboBox&>(b).m_dropDownList ? L"DropDownList" : L"DropDown"; }},
    };
    return kTable;
}

bool XmlComboBox::SetItemsProp(const std::wstring& v)
{
    m_itemsCache = v;
    fillItems();
    return true;
}

std::wstring XmlComboBox::GetItemsProp() const
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

bool XmlComboBox::SetTextProp(const std::wstring& v)
{
    m_text = v;
    if (m_host != nullptr)
        m_host->setText(m_id, v);
    return true;
}

std::wstring XmlComboBox::GetTextProp() const
{
    return m_host != nullptr ? m_host->getText(m_id) : m_text;
}

bool XmlComboBox::SetSelectedIndexProp(const std::wstring& v)
{
    m_selectedIndex = xmlprop2::ToInt(v, -1);
    if (m_host != nullptr)
        m_host->setCurSel(m_id, m_selectedIndex);
    return true;
}

std::wstring XmlComboBox::GetSelectedIndexProp() const
{
    if (m_host == nullptr)
        return xmlprop2::FromInt(m_selectedIndex);
    return xmlprop2::FromInt(m_host->getCurSel(m_id));
}

bool XmlComboBox::SetDropDownStyleProp(const std::wstring& v)
{
    // DropDownList：只能选；DropDown：可编辑（也能手打文字）
    const std::wstring t = xmlprop2::Trim(v);
    const bool listOnly = (_wcsicmp(t.c_str(), L"DropDownList") == 0 ||
                           _wcsicmp(t.c_str(), L"list") == 0);
    if (listOnly != m_dropDownList)
    {
        m_dropDownList = listOnly;
        rebuild();
    }
    return true;
}
