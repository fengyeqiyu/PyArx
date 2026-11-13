// XmlPathView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlPathView.h"

void XmlPathView::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 19, x, y, w, h);
    fillItems();
    if (m_selectedIndex > 0)
        host->setListSelected(id, m_selectedIndex);
}

void XmlPathView::fillItems()
{
    if (m_host == nullptr || m_items.empty()) return;
    m_host->clearListItems(m_id);
    std::wstring cur;
    for (wchar_t ch : m_items)
    {
        if (ch == L';' || ch == L'\n' || ch == L'\r')
        {
            if (!cur.empty())
                m_host->addListItem(m_id, cur);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }
    if (!cur.empty())
        m_host->addListItem(m_id, cur);
}

const std::vector<XmlPropEntry>& XmlPathView::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPathView&>(b);
             c.m_items = v;
             c.fillItems();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlPathView&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPathView&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setListSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlPathView&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getListSelected(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
    };
    return kTable;
}