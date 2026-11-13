// XamlPageIndicator.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlPageIndicator.h"

void XamlPageIndicator::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 5, x, y, w, h);
    host->setPageCount(id, m_count);
    if (m_selectedIndex > 0)
        host->setPageIndex(id, m_selectedIndex);
}

const std::vector<XamlPropEntry>& XamlPageIndicator::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Count", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlPageIndicator&>(b);
             c.m_count = xamlprop2::ToInt(v, 3);
             if (c.m_host != nullptr) c.m_host->setPageCount(c.m_id, c.m_count);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlPageIndicator&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageCount(c.m_id) : c.m_count);
         }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlPageIndicator&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setPageIndex(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlPageIndicator&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageIndex(c.m_id) : c.m_selectedIndex);
         }},
    };
    return kTable;
}