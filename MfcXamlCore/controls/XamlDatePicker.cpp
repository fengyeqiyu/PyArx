// XamlDatePicker.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlDatePicker.h"

void XamlDatePicker::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addDatePicker(id, x, y, w, h);
    if (!m_value.empty()) host->setText(id, m_value);
}

const std::vector<XamlPropEntry>& XamlDatePicker::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Value", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDatePicker&>(b);
             c.m_value = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDatePicker&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_value;
         }},
    };
    return kTable;
}