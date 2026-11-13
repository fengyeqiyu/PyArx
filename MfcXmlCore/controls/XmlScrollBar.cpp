// XmlScrollBar.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlScrollBar.h"

void XmlScrollBar::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    if (!m_rangeSet)
    {
        m_vmin = minValue;
        m_vmax = maxValue;
    }
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::ScrollBar), x, y, w, h);
    host->setScrollBarRange(id, m_vmin, m_vmax, m_pg, m_vert);
    if (m_valueSet)
        host->setScrollBarValue(id, m_value);
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlScrollBar::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Minimum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlScrollBar&>(b);
             s.m_vmin = xmlprop2::ToInt(v, s.m_vmin);
             s.m_rangeSet = true;
             s.pushRange();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlScrollBar&>(b).m_vmin);
         }},
        {L"Maximum", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlScrollBar&>(b);
             s.m_vmax = xmlprop2::ToInt(v, s.m_vmax);
             s.m_rangeSet = true;
             s.pushRange();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlScrollBar&>(b).m_vmax);
         }},
        {L"Value", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlScrollBar&>(b);
             s.m_value = xmlprop2::ToInt(v, s.m_value);
             s.m_valueSet = true;
             if (s.m_host != nullptr)
                 s.m_host->setScrollBarValue(s.m_id, s.m_value);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& s = static_cast<const XmlScrollBar&>(b);
             return xmlprop2::FromInt(s.m_host != nullptr ? s.m_host->getScrollBarValue(s.m_id) : s.m_value);
         }},
        {L"PageSize", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlScrollBar&>(b);
             s.m_pg = xmlprop2::ToInt(v, s.m_pg);
             if (s.m_pg < 1)
                 s.m_pg = 1;
             s.pushRange();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlScrollBar&>(b).m_pg);
         }},
        {L"Orientation", nullptr, XmlPropType::Enum, L"horizontal|vertical",
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& s = static_cast<XmlScrollBar&>(b);
             const std::wstring t = xmlprop2::Trim(v);
             s.m_vert = (_wcsicmp(t.c_str(), L"vertical") == 0);
             s.pushRange();
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return std::wstring(static_cast<const XmlScrollBar&>(b).m_vert ? L"vertical" : L"horizontal");
         }},
    };
    return kTable;
}
