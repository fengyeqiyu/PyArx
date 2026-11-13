// XamlToolBar.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlToolBar.h"

void XamlToolBar::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 11, x, y, w, h);
    applyItems();
    if (m_selectedIndex >= 0)
        host->setToolBarSelected(id, m_selectedIndex);
}

void XamlToolBar::applyItems()
{
    if (m_host == nullptr || m_items.empty()) return;
    std::wstring cur;
    for (wchar_t ch : m_items)
    {
        if (ch == L'|' || ch == L'\n' || ch == L'\r')
        {
            if (!cur.empty())
                m_host->addToolBarItem(m_id, cur);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }
    if (!cur.empty())
        m_host->addToolBarItem(m_id, cur);
}

const std::vector<XamlPropEntry>& XamlToolBar::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlToolBar&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlToolBar&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlToolBar&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setToolBarSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlToolBar&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getToolBarSelected(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}