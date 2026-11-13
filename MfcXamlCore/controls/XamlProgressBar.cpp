// XamlProgressBar.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlProgressBar.h"

void XamlProgressBar::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addProgress(id, x, y, w, h);
    host->setRange(id, minValue, maxValue);
    host->setPos(id, m_valueSet ? m_value : minValue);
}

// 属性表：进度条没有自身交互，Value 只由外部驱动。
const std::vector<XamlPropEntry>& XamlProgressBar::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Minimum", L"Min", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XamlProgressBar&>(b);
             p.m_min = xamlprop2::ToInt(v, p.m_min);
             if (p.m_host != nullptr)
                 p.m_host->setRange(p.m_id, p.m_min, p.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlProgressBar&>(b).m_min);
         }},
        {L"Maximum", L"Max", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XamlProgressBar&>(b);
             p.m_max = xamlprop2::ToInt(v, p.m_max);
             if (p.m_host != nullptr)
                 p.m_host->setRange(p.m_id, p.m_min, p.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlProgressBar&>(b).m_max);
         }},
        {L"Value", L"Position", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& p = static_cast<XamlProgressBar&>(b);
             p.m_value = xamlprop2::ToInt(v, p.m_value);
             p.m_valueSet = true;
             if (p.m_host != nullptr)
                 p.m_host->setPos(p.m_id, p.m_value);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& p = static_cast<const XamlProgressBar&>(b);
             return xamlprop2::FromInt(p.m_host != nullptr ? p.m_host->getPos(p.m_id) : p.m_value);
         }},
    };
    return kTable;
}
