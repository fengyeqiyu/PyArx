// XmlRangeSlider.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlRangeSlider.h"

void XmlRangeSlider::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::Range), x, y, w, h);
    host->setRange(id, m_min, m_max);
    host->setRangeValues(id, m_lowSet ? m_low : minValue, m_highSet ? m_high : maxValue);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlRangeSlider::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"LowValue", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XmlRangeSlider&>(b);
             r.m_low = xmlprop2::ToInt(v, r.m_low);
             r.m_lowSet = true;
             if (r.m_host != nullptr)
                 r.m_host->setRangeValues(r.m_id, r.m_low, r.m_host->getRangeHigh(r.m_id));
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& r = static_cast<const XmlRangeSlider&>(b);
             return xmlprop2::FromInt(r.m_host != nullptr ? r.m_host->getRangeLow(r.m_id) : r.m_low);
         }},
        {L"HighValue", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XmlRangeSlider&>(b);
             r.m_high = xmlprop2::ToInt(v, r.m_high);
             r.m_highSet = true;
             if (r.m_host != nullptr)
                 r.m_host->setRangeValues(r.m_id, r.m_host->getRangeLow(r.m_id), r.m_high);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& r = static_cast<const XmlRangeSlider&>(b);
             return xmlprop2::FromInt(r.m_host != nullptr ? r.m_host->getRangeHigh(r.m_id) : r.m_high);
         }},
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XmlRangeSlider&>(b);
             r.m_min = xmlprop2::ToInt(v, r.m_min);
             if (r.m_host != nullptr)
                 r.m_host->setRange(r.m_id, r.m_min, r.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlRangeSlider&>(b).m_min);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XmlRangeSlider&>(b);
             r.m_max = xmlprop2::ToInt(v, r.m_max);
             if (r.m_host != nullptr)
                 r.m_host->setRange(r.m_id, r.m_min, r.m_max);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlRangeSlider&>(b).m_max);
         }},
    };
    return kTable;
}
