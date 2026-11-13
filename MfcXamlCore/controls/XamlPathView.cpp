// XamlPathView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlPathView.h"

void XamlPathView::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 19, x, y, w, h);
    fillItems();
    if (m_selectedIndex > 0)
        host->setListSelected(id, m_selectedIndex);
}

void XamlPathView::fillItems()
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

const std::vector<XamlPropEntry>& XamlPathView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Items", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlPathView&>(b);
             c.m_items = v;
             c.fillItems();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlPathView&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlPathView&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setListSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlPathView&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getListSelected(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
    };
    return kTable;
}