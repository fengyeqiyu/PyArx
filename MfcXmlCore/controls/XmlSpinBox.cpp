// XmlSpinBox.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlSpinBox.h"

void XmlSpinBox::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::Spin), x, y, w, h);
    host->setSpinRange(id, m_min, m_max);
    if (m_valueSet)
        host->setSpinValue(id, m_value);
    else
        (void)minValue, (void)maxValue;
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlSpinBox::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSpinBox&>(b);
             s.m_min = xmlprop2::ToInt(v, s.m_min);
             if (s.m_host != nullptr)
                 s.m_host->setSpinRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlSpinBox&>(b).m_min);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSpinBox&>(b);
             s.m_max = xmlprop2::ToInt(v, s.m_max);
             if (s.m_host != nullptr)
                 s.m_host->setSpinRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlSpinBox&>(b).m_max);
         }},
        {L"Value", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlSpinBox&>(b);
             s.m_value = xmlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setSpinValue(s.m_id, s.m_value);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& s = static_cast<const XmlSpinBox&>(b);
             return xmlprop2::FromInt(s.m_host != nullptr ? s.m_host->getSpinValue(s.m_id) : s.m_value);
         }},
    };
    return kTable;
}
