// XmlPageView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlPageView.h"

void XmlPageView::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 17, x, y, w, h);
    if (m_selectedIndex > 0) host->setPageIndex(id, m_selectedIndex);
}

const std::vector<XmlPropEntry>& XmlPageView::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPageView&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setPageIndex(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlPageView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageIndex(c.m_id) : c.m_selectedIndex);
         }},
        {L"PageCount", nullptr, XmlPropType::Int, nullptr, nullptr,
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlPageView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageCount(c.m_id) : 0);
         }},
    };
    return kTable;
}