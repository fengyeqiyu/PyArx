// XmlToggleButton.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlToggleButton.h"

void XmlToggleButton::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addToggle(id, m_text, x, y, w, h);
    if (m_checked) host->setCheck(id, true);
}

const std::vector<XmlPropEntry>& XmlToggleButton::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToggleButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToggleButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Checked", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToggleButton&>(b);
             c.m_checked = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_checked);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToggleButton&>(b);
             const bool on = c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_checked;
             return xmlprop2::FromBool(on);
         }},
    };
    return kTable;
}