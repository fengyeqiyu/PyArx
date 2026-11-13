// XmlPageIndicator.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlPageIndicator.h"

void XmlPageIndicator::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 5, x, y, w, h);
    host->setPageCount(id, m_count);
    if (m_selectedIndex > 0)
        host->setPageIndex(id, m_selectedIndex);
}

const std::vector<XmlPropEntry>& XmlPageIndicator::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Count", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPageIndicator&>(b);
             c.m_count = xmlprop2::ToInt(v, 3);
             if (c.m_host != nullptr) c.m_host->setPageCount(c.m_id, c.m_count);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlPageIndicator&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageCount(c.m_id) : c.m_count);
         }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlPageIndicator&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setPageIndex(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlPageIndicator&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageIndex(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}