// XamlTabControl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlTabControl.h"

void XamlTabControl::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addTab(id, x, y, w, h);
    addPages();
}

void XamlTabControl::addPages()
{
    if (m_host == nullptr || m_pages.empty()) return;
    size_t start = 0;
    while (start <= m_pages.size())
    {
        const size_t bar = m_pages.find(L'|', start);
        const std::wstring item = (bar == std::wstring::npos)
            ? m_pages.substr(start)
            : m_pages.substr(start, bar - start);
        if (!item.empty())
            m_host->addTabItem(m_id, item);
        if (bar == std::wstring::npos)
            break;
        start = bar + 1;
    }
}

const std::vector<XamlPropEntry>& XamlTabControl::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Pages", nullptr, XamlPropType::Items, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlTabControl&>(b);
             c.m_pages = v;
             c.addPages();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTabControl&>(b).m_pages; }},
    };
    return kTable;
}