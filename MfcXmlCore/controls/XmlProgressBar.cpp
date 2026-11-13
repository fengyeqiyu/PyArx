// XmlProgressBar.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlProgressBar.h"

void XmlProgressBar::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addProgress(id, x, y, w, h);
    host->setRange(id, minValue, maxValue);
    host->setPos(id, m_valueSet ? m_value : minValue);
}

// 属性表：进度条没有自身交互，Value 只由外部驱动。
const std::vector<XmlPropEntry>& XmlProgressBar::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XmlProgressBar&>(b);
             p.m_min = xmlprop2::ToInt(v, p.m_min);
             if (p.m_host != nullptr)
                 p.m_host->setRange(p.m_id, p.m_min, p.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlProgressBar&>(b).m_min);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XmlProgressBar&>(b);
             p.m_max = xmlprop2::ToInt(v, p.m_max);
             if (p.m_host != nullptr)
                 p.m_host->setRange(p.m_id, p.m_min, p.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlProgressBar&>(b).m_max);
         }},
        {L"Value", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XmlProgressBar&>(b);
             p.m_value = xmlprop2::ToInt(v, p.m_value);
             p.m_valueSet = true;
             if (p.m_host != nullptr)
                 p.m_host->setPos(p.m_id, p.m_value);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& p = static_cast<const XmlProgressBar&>(b);
             return xmlprop2::FromInt(p.m_host != nullptr ? p.m_host->getPos(p.m_id) : p.m_value);
         }},
    };
    return kTable;
}
