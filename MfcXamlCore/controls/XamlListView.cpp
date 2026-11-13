// XamlListView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlListView.h"

void XamlListView::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 16, x, y, w, h);
    fillItems();
    if (m_selectedIndex >= 0)
        host->setListSelected(id, m_selectedIndex);
}

void XamlListView::fillItems()
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

const std::vector<XamlPropEntry>& XamlListView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListView&>(b);
             c.m_items = v;
             c.fillItems();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlListView&>(b).m_items; }},
        {L"ItemHeight", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListView&>(b);
             c.m_itemH = xamlprop2::ToInt(v, 44);
             if (c.m_itemH < 20) c.m_itemH = 20;
             return true;
         },
         [](const XamlMfcControlBase& b) { return std::to_wstring(static_cast<const XamlListView&>(b).m_itemH); }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListView&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, -1);
             if (c.m_host != nullptr) c.m_host->setListSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlListView&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getListSelected(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
        {L"ItemCount", nullptr, XamlPropType::Int, nullptr, nullptr,
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlListView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getListItemCount(c.m_id) : 0);
         }},
    };
    return kTable;
}