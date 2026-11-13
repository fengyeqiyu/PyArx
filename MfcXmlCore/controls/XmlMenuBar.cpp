// XmlMenuBar.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlMenuBar.h"

void XmlMenuBar::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 14, x, y, w, h);
    applyItems();
    if (m_selectedIndex >= 0)
        host->setMenuBarSelected(id, m_selectedIndex);
}

void XmlMenuBar::applyItems()
{
    if (m_host == nullptr || m_items.empty()) return;
    for (const auto& s : xmlSplitItems(m_items))
        m_host->addMenuBarItem(m_id, s);
}

const std::vector<XmlPropEntry>& XmlMenuBar::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlMenuBar&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlMenuBar&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlMenuBar&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setMenuBarSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlMenuBar&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getMenuBarSelected(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}