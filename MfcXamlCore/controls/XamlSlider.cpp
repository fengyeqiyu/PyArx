// XamlSlider.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlSlider.h"

void XamlSlider::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSlider(id, minValue, maxValue, x, y, w, h);
    if (m_valueSet)
        host->setPos(id, m_value);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XamlPropEntry>& XamlSlider::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Minimum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSlider&>(b);
             s.m_min = xamlprop2::ToInt(v, s.m_min);
             if (s.m_host != nullptr)
                 s.m_host->setRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlSlider&>(b).m_min);
         }},
        {L"Maximum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSlider&>(b);
             s.m_max = xamlprop2::ToInt(v, s.m_max);
             if (s.m_host != nullptr)
                 s.m_host->setRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlSlider&>(b).m_max);
         }},
        {L"Value", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSlider&>(b);
             s.m_value = xamlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setPos(s.m_id, s.m_value);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& s = static_cast<const XamlSlider&>(b);
             return xamlprop2::FromInt(s.m_host != nullptr ? s.m_host->getPos(s.m_id) : s.m_value);
         }},
    };
    return kTable;
}
