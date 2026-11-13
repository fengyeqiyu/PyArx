// XmlTextBlock.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlTextBlock.h"

void XmlTextBlock::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addLabel(id, m_text, x, y, w, h);
}

const std::vector<XmlPropEntry>& XmlTextBlock::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlTextBlock&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlTextBlock&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
    };
    return kTable;
}