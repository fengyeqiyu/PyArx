// XamlCheckBox.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlCheckBox.h"

void XamlCheckBox::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCheckBox(id, m_text, x, y, w, h);
    if (m_checked) host->setCheck(id, true);
}

const std::vector<XamlPropEntry>& XamlCheckBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCheckBox&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlCheckBox&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Checked", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCheckBox&>(b);
             c.m_checked = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_checked);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlCheckBox&>(b);
             const bool on = c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_checked;
             return xamlprop2::FromBool(on);
         }},
    };
    return kTable;
}