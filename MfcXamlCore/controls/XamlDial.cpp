// XamlDial.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlDial.h"

void XamlDial::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    if (!m_rangeSet)
    {
        m_vmin = minValue;
        m_vmax = maxValue;
    }
    host->addSkinBox(id, XamlSkinId(XamlSkinKind::Dial), x, y, w, h);
    host->setRange(id, m_vmin, m_vmax);
    host->setDial(id, m_valueSet ? m_value : m_vmin);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XamlPropEntry>& XamlDial::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Minimum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XamlDial&>(b);
             d.m_vmin = xamlprop2::ToInt(v, d.m_vmin);
             d.m_rangeSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setRange(d.m_id, d.m_vmin, d.m_vmax);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlDial&>(b).m_vmin);
         }},
        {L"Maximum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XamlDial&>(b);
             d.m_vmax = xamlprop2::ToInt(v, d.m_vmax);
             d.m_rangeSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setRange(d.m_id, d.m_vmin, d.m_vmax);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlDial&>(b).m_vmax);
         }},
        {L"Value", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& d = static_cast<XamlDial&>(b);
             d.m_value = xamlprop2::ToInt(v, d.m_value);
             d.m_valueSet = true;
             if (d.m_host != nullptr)
                 d.m_host->setDial(d.m_id, d.m_value);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& d = static_cast<const XamlDial&>(b);
             return xamlprop2::FromInt(d.m_host != nullptr ? d.m_host->getDial(d.m_id) : d.m_value);
         }},
    };
    return kTable;
}
