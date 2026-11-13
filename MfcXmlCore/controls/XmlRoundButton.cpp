// XmlRoundButton.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlRoundButton.h"

void XmlRoundButton::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addButton(id, m_text, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlRoundButton::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlRoundButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlRoundButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Radius", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlRoundButton&>(b);
             c.m_radius = xmlprop2::ToInt(v, c.m_radius);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlRoundButton&>(b).m_radius);
         }},
    };
    return kTable;
}
