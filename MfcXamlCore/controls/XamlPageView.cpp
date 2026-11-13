// XamlPageView.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlPageView.h"

void XamlPageView::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 17, x, y, w, h);
    if (m_selectedIndex > 0) host->setPageIndex(id, m_selectedIndex);
}

const std::vector<XamlPropEntry>& XamlPageView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlPageView&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, 0);
             if (c.m_host != nullptr) c.m_host->setPageIndex(c.m_id, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlPageView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageIndex(c.m_id) : c.m_selectedIndex);
         }},
        {L"PageCount", nullptr, XamlPropType::Int, nullptr, nullptr,
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlPageView&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getPageCount(c.m_id) : 0);
         }},
    };
    return kTable;
}