// XmlGroupBox.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlGroupBox.h"

void XmlGroupBox::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addGroup(id, m_header, x, y, w, h);
}

const std::vector<XmlPropEntry>& XmlGroupBox::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Header", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGroupBox&>(b);
             c.m_header = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlGroupBox&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_header;
         }},
    };
    return kTable;
}