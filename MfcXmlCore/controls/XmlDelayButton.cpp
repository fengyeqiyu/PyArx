// XmlDelayButton.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlDelayButton.h"

void XmlDelayButton::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addButton(id, m_text, x, y, w, h);
    if (m_checked) host->setCheck(id, true);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlDelayButton::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDelayButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDelayButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Delay", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDelayButton&>(b);
             c.m_delay = xmlprop2::ToInt(v, c.m_delay);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlDelayButton&>(b).m_delay);
         }},
        {L"Checked", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDelayButton&>(b);
             c.m_checked = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_checked);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDelayButton&>(b);
             return xmlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_checked);
         }},
    };
    return kTable;
}
