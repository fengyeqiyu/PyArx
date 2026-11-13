// XmlTabControl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlTabControl.h"

void XmlTabControl::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addTab(id, x, y, w, h);
    addPages();
}

void XmlTabControl::addPages()
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

const std::vector<XmlPropEntry>& XmlTabControl::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Pages", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlTabControl&>(b);
             c.m_pages = v;
             c.addPages();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTabControl&>(b).m_pages; }},
    };
    return kTable;
}