// XmlDatePicker.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlDatePicker.h"

void XmlDatePicker::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addDatePicker(id, x, y, w, h);
    if (!m_value.empty()) host->setText(id, m_value);
}

const std::vector<XmlPropEntry>& XmlDatePicker::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Value", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDatePicker&>(b);
             c.m_value = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDatePicker&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_value;
         }},
    };
    return kTable;
}