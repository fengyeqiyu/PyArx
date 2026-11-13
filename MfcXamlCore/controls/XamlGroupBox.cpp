// XamlGroupBox.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlGroupBox.h"

void XamlGroupBox::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addGroup(id, m_header, x, y, w, h);
}

const std::vector<XamlPropEntry>& XamlGroupBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Header", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlGroupBox&>(b);
             c.m_header = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlGroupBox&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_header;
         }},
    };
    return kTable;
}