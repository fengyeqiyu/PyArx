// XmlHyperlink.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlHyperlink.h"

void XmlHyperlink::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addHyperlink(id, m_url, x, y, w, h);
    if (!m_text.empty()) host->setText(id, m_text);
}

const std::vector<XmlPropEntry>& XmlHyperlink::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlHyperlink&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlHyperlink&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Url", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlHyperlink&>(b);
             c.m_url = v;
             if (c.m_host != nullptr) c.m_host->setUrl(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlHyperlink&>(b).m_url; }},
    };
    return kTable;
}