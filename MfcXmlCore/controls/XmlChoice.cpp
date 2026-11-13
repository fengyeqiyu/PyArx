// XmlChoice.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlChoice.h"

void XmlChoice::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addChoice(id, x, y, w, h);
    if (!m_items.empty()) fillItems();
}

void XmlChoice::fillItems()
{
    if (m_host == nullptr) return;
    m_host->clearStrings(m_id);
    size_t start = 0;
    while (start <= m_items.size())
    {
        const size_t bar = m_items.find(L'|', start);
        const std::wstring item = (bar == std::wstring::npos)
            ? m_items.substr(start)
            : m_items.substr(start, bar - start);
        if (!item.empty())
            m_host->addString(m_id, item);
        if (bar == std::wstring::npos)
            break;
        start = bar + 1;
    }
}

const std::vector<XmlPropEntry>& XmlChoice::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Items", nullptr, XmlPropType::Items, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlChoice&>(b);
             c.m_items = v;
             if (c.m_host != nullptr) c.fillItems();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlChoice&>(b);
             if (c.m_host == nullptr) return c.m_items;
             std::wstring out;
             const int n = c.m_host->countStrings(c.m_id);
             for (int i = 0; i < n; ++i)
             {
                 if (i > 0) out += L"|";
                 out += c.m_host->getString(c.m_id, i);
             }
             return out;
         }},
    };
    return kTable;
}