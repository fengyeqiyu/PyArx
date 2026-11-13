// XmlGridView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlGridView.h"

void XmlGridView::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 18, x, y, w, h);
    fillItems();
    if (m_cellW > 0 || m_cellH > 0)
        host->setGridCellSize(id, m_cellW, m_cellH);
    if (m_selectedIndex > 0)
        host->setListSelected(id, m_selectedIndex);
}

void XmlGridView::fillItems()
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

const std::vector<XmlPropEntry>& XmlGridView::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGridView&>(b);
             c.m_items = v;
             c.fillItems();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlGridView&>(b).m_items; }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGridView&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setListSelected(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlGridView&>(b);
             const int i = c.m_host != nullptr ? c.m_host->getListSelected(c.m_id) : c.m_selectedIndex;
             return std::to_wstring(i);
         }},
        {L"CellWidth", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGridView&>(b);
             c.m_cellW = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setGridCellSize(c.m_id, c.m_cellW, c.m_cellH);
             return true;
         },
         [](const XmlMfcControlBase& b) { return std::to_wstring(static_cast<const XmlGridView&>(b).m_cellW); }},
        {L"CellHeight", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGridView&>(b);
             c.m_cellH = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setGridCellSize(c.m_id, c.m_cellW, c.m_cellH);
             return true;
         },
         [](const XmlMfcControlBase& b) { return std::to_wstring(static_cast<const XmlGridView&>(b).m_cellH); }},
        {L"ItemCount", nullptr, XmlPropType::Int, nullptr, nullptr,
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlGridView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getListItemCount(c.m_id) : 0);
         }},
    };
    return kTable;
}