// XmlSlider.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlSlider.h"

void XmlSlider::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSlider(id, minValue, maxValue, x, y, w, h);
    if (m_valueSet)
        host->setPos(id, m_value);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlSlider::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSlider&>(b);
             s.m_min = xmlprop2::ToInt(v, s.m_min);
             if (s.m_host != nullptr)
                 s.m_host->setRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlSlider&>(b).m_min);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSlider&>(b);
             s.m_max = xmlprop2::ToInt(v, s.m_max);
             if (s.m_host != nullptr)
                 s.m_host->setRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlSlider&>(b).m_max);
         }},
        {L"Value", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSlider&>(b);
             s.m_value = xmlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setPos(s.m_id, s.m_value);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& s = static_cast<const XmlSlider&>(b);
             return xmlprop2::FromInt(s.m_host != nullptr ? s.m_host->getPos(s.m_id) : s.m_value);
         }},
    };
    return kTable;
}
