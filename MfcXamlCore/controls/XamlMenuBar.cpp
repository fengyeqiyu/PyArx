// XamlMenuBar.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlMenuBar.h"

void XamlMenuBar::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 14, x, y, w, h);
    applyItems();
    if (m_selectedIndex >= 0)
        host->setMenuBarSelected(id, m_selectedIndex);
}

void XamlMenuBar::applyItems()
{
    if (m_host == nullptr || m_items.empty()) return;
    for (const auto& s : xamlSplitItems(m_items))
        m_host->addMenuBarItem(m_id, s);
}

const std::vector<XamlPropEntry>& XamlMenuBar::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlMenuBar&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlMenuBar&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlMenuBar&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setMenuBarSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlMenuBar&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getMenuBarSelected(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}