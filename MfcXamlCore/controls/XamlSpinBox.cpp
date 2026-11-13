// XamlSpinBox.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlSpinBox.h"

void XamlSpinBox::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, XamlSkinId(XamlSkinKind::Spin), x, y, w, h);
    host->setSpinRange(id, m_min, m_max);
    if (m_valueSet)
        host->setSpinValue(id, m_value);
    else
        (void)minValue, (void)maxValue;
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XamlPropEntry>& XamlSpinBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Minimum", L"Min", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSpinBox&>(b);
             s.m_min = xamlprop2::ToInt(v, s.m_min);
             if (s.m_host != nullptr)
                 s.m_host->setSpinRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlSpinBox&>(b).m_min);
         }},
        {L"Maximum", L"Max", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSpinBox&>(b);
             s.m_max = xamlprop2::ToInt(v, s.m_max);
             if (s.m_host != nullptr)
                 s.m_host->setSpinRange(s.m_id, s.m_min, s.m_max);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlSpinBox&>(b).m_max);
         }},
        {L"Value", L"Position", XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlSpinBox&>(b);
             s.m_value = xamlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setSpinValue(s.m_id, s.m_value);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& s = static_cast<const XamlSpinBox&>(b);
             return xamlprop2::FromInt(s.m_host != nullptr ? s.m_host->getSpinValue(s.m_id) : s.m_value);
         }},
    };
    return kTable;
}
