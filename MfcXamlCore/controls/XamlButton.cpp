// XamlButton.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlButton.h"

void XamlButton::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addButton(id, m_text, x, y, w, h);
}

const std::vector<XamlPropEntry>& XamlButton::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
    };
    return kTable;
}