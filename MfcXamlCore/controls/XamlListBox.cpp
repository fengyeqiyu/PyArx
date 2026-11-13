// XamlListBox.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlListBox.h"

void XamlListBox::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addListBoxAdvanced(id, m_multiple, x, y, w, h);
    fillItems();
    if (m_selectedIndex >= 0)
        host->setCurSel(id, m_selectedIndex);
}

void XamlListBox::fillItems()
{
    if (m_host == nullptr) return;
    m_host->clearStrings(m_id);
    size_t start = 0;
    while (start <= m_itemsCache.size())
    {
        const size_t bar = m_itemsCache.find(L'|', start);
        const std::wstring item = (bar == std::wstring::npos)
            ? m_itemsCache.substr(start)
            : m_itemsCache.substr(start, bar - start);
        if (!item.empty())
            m_host->addString(m_id, item);
        if (bar == std::wstring::npos)
            break;
        start = bar + 1;
    }
}

void XamlListBox::rebuild()
{
    if (m_host == nullptr) return;
    int x = 0, y = 0, w = 0, h = 0;
    getBounds(x, y, w, h);
    if (w <= 0 || h <= 0) return;
    m_host->addListBoxAdvanced(m_id, m_multiple, x, y, w, h);
    fillItems();
    if (m_selectedIndex >= 0)
        m_host->setCurSel(m_id, m_selectedIndex);
    pushAppearance();
}

const std::vector<XamlPropEntry>& XamlListBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListBox&>(b);
             c.m_itemsCache = v;
             if (c.m_host != nullptr) c.fillItems();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlListBox&>(b);
             if (c.m_host == nullptr) return c.m_itemsCache;
             std::wstring out;
             const int n = c.m_host->countStrings(c.m_id);
             for (int i = 0; i < n; ++i)
             {
                 if (i > 0) out += L"|";
                 out += c.m_host->getString(c.m_id, i);
             }
             return out;
         }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListBox&>(b);
             c.m_selectedIndex = v.empty() ? -1 : xamlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setCurSel(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlListBox&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getCurSel(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
        {L"SelectionMode", nullptr, XamlPropType::Enum, L"Single|Multiple",
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListBox&>(b);
             const bool multiple = (_wcsicmp(v.c_str(), L"Multiple") == 0 ||
                                    _wcsicmp(v.c_str(), L"MultiExtended") == 0);
             if (multiple != c.m_multiple)
             {
                 c.m_multiple = multiple;
                 c.rebuild();
             }
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return static_cast<const XamlListBox&>(b).m_multiple ? L"Multiple" : L"Single";
         }},
    };
    return kTable;
}