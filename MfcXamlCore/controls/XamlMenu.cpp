// XamlMenu.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlMenu.h"

void XamlMenu::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 13, x, y, w, h);
    applyItems();
}

void XamlMenu::applyItems()
{
    if (m_host != nullptr && !m_items.empty())
        m_host->setMenuItems(m_id, xamlSplitItems(m_items));
}

const std::vector<XamlPropEntry>& XamlMenu::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlMenu&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlMenu&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr, nullptr,
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlMenu&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getMenuSelected(c.m_id) : -1);
         }},
    };
    return kTable;
}