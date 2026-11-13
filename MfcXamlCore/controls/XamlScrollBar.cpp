// XamlScrollBar.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlScrollBar.h"

void XamlScrollBar::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    if (!m_rangeSet)
    {
        m_vmin = minValue;
        m_vmax = maxValue;
    }
    host->addSkinBox(id, XamlSkinId(XamlSkinKind::ScrollBar), x, y, w, h);
    host->setScrollBarRange(id, m_vmin, m_vmax, m_pg, m_vert);
    if (m_valueSet)
        host->setScrollBarValue(id, m_value);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XamlPropEntry>& XamlScrollBar::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Minimum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlScrollBar&>(b);
             s.m_vmin = xamlprop2::ToInt(v, s.m_vmin);
             s.m_rangeSet = true;
             s.pushRange();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlScrollBar&>(b).m_vmin);
         }},
        {L"Maximum", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlScrollBar&>(b);
             s.m_vmax = xamlprop2::ToInt(v, s.m_vmax);
             s.m_rangeSet = true;
             s.pushRange();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlScrollBar&>(b).m_vmax);
         }},
        {L"Value", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlScrollBar&>(b);
             s.m_value = xamlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setScrollBarValue(s.m_id, s.m_value);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& s = static_cast<const XamlScrollBar&>(b);
             return xamlprop2::FromInt(s.m_host != nullptr ? s.m_host->getScrollBarValue(s.m_id) : s.m_value);
         }},
        {L"PageSize", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlScrollBar&>(b);
             s.m_pg = xamlprop2::ToInt(v, s.m_pg);
             if (s.m_pg < 1)
                 s.m_pg = 1;
             s.pushRange();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlScrollBar&>(b).m_pg);
         }},
        {L"Orientation", nullptr, XamlPropType::Enum, L"horizontal|vertical",
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XamlScrollBar&>(b);
             const std::wstring t = xamlprop2::Trim(v);
             s.m_vert = (_wcsicmp(t.c_str(), L"vertical") == 0);
             s.pushRange();
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return std::wstring(static_cast<const XamlScrollBar&>(b).m_vert ? L"vertical" : L"horizontal");
         }},
    };
    return kTable;
}
