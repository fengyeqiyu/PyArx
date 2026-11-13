// XmlToolBar.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlToolBar.h"

void XmlToolBar::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 11, x, y, w, h);
    applyItems();
    if (m_selectedIndex >= 0)
        host->setToolBarSelected(id, m_selectedIndex);
}

void XmlToolBar::applyItems()
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

const std::vector<XmlPropEntry>& XmlToolBar::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolBar&>(b);
             c.m_items = v;
             c.applyItems();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlToolBar&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolBar&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setToolBarSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToolBar&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getToolBarSelected(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}