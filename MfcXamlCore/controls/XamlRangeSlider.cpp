// XamlRangeSlider.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlRangeSlider.h"

void XamlRangeSlider::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, XamlSkinId(XamlSkinKind::Range), x, y, w, h);
    host->setRange(id, m_min, m_max);
    host->setRangeValues(id, m_lowSet ? m_low : minValue, m_highSet ? m_high : maxValue);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XamlPropEntry>& XamlRangeSlider::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"LowValue", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XamlRangeSlider&>(b);
             r.m_low = xamlprop2::ToInt(v, r.m_low);
             r.m_lowSet = true;
             if (r.m_host != nullptr)
                 r.m_host->setRangeValues(r.m_id, r.m_low, r.m_host->getRangeHigh(r.m_id));
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& r = static_cast<const XamlRangeSlider&>(b);
             return xamlprop2::FromInt(r.m_host != nullptr ? r.m_host->getRangeLow(r.m_id) : r.m_low);
         }},
        {L"HighValue", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XamlRangeSlider&>(b);
             r.m_high = xamlprop2::ToInt(v, r.m_high);
             r.m_highSet = true;
             if (r.m_host != nullptr)
                 r.m_host->setRangeValues(r.m_id, r.m_host->getRangeLow(r.m_id), r.m_high);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& r = static_cast<const XamlRangeSlider&>(b);
             return xamlprop2::FromInt(r.m_host != nullptr ? r.m_host->getRangeHigh(r.m_id) : r.m_high);
         }},
        {L"Minimum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XamlRangeSlider&>(b);
             r.m_min = xamlprop2::ToInt(v, r.m_min);
             if (r.m_host != nullptr)
                 r.m_host->setRange(r.m_id, r.m_min, r.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlRangeSlider&>(b).m_min);
         }},
        {L"Maximum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& r = static_cast<XamlRangeSlider&>(b);
             r.m_max = xamlprop2::ToInt(v, r.m_max);
             if (r.m_host != nullptr)
                 r.m_host->setRange(r.m_id, r.m_min, r.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlRangeSlider&>(b).m_max);
         }},
    };
    return kTable;
}
