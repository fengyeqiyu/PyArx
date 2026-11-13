// XmlDial.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlDial.h"

void XmlDial::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    if (!m_rangeSet)
    {
        m_vmin = minValue;
        m_vmax = maxValue;
    }
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::Dial), x, y, w, h);
    host->setRange(id, m_vmin, m_vmax);
    host->setDial(id, m_valueSet ? m_value : m_vmin);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlDial::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XmlDial&>(b);
             d.m_vmin = xmlprop2::ToInt(v, d.m_vmin);
             d.m_rangeSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setRange(d.m_id, d.m_vmin, d.m_vmax);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlDial&>(b).m_vmin);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XmlDial&>(b);
             d.m_vmax = xmlprop2::ToInt(v, d.m_vmax);
             d.m_rangeSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setRange(d.m_id, d.m_vmin, d.m_vmax);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlDial&>(b).m_vmax);
         }},
        {L"Value", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XmlDial&>(b);
             d.m_value = xmlprop2::ToInt(v, d.m_value);
             d.m_valueSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setDial(d.m_id, d.m_value);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& d = static_cast<const XmlDial&>(b);
             return xmlprop2::FromInt(d.m_host != nullptr ? d.m_host->getDial(d.m_id) : d.m_value);
         }},
    };
    return kTable;
}
