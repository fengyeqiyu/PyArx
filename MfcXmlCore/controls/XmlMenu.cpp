// XmlMenu.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlMenu.h"

void XmlMenu::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 13, x, y, w, h);
    applyItems();
}

void XmlMenu::applyItems()
{
    if (m_host != nullptr && !m_items.empty())
        m_host->setMenuItems(m_id, xmlSplitItems(m_items));
}

const std::vector<XmlPropEntry>& XmlMenu::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlMenu&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlMenu&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr, nullptr,
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlMenu&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getMenuSelected(c.m_id) : -1);
         }},
    };
    return kTable;
}