// XamlHyperlink.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlHyperlink.h"

void XamlHyperlink::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addHyperlink(id, m_url, x, y, w, h);
    if (!m_text.empty()) host->setText(id, m_text);
}

const std::vector<XamlPropEntry>& XamlHyperlink::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlHyperlink&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlHyperlink&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Url", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlHyperlink&>(b);
             c.m_url = v;
             if (c.m_host != nullptr) c.m_host->setUrl(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlHyperlink&>(b).m_url; }},
    };
    return kTable;
}