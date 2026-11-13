// XmlTumbler.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlTumbler.h"

void XmlTumbler::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, XmlSkinId(XmlSkinKind::Tumbler), x, y, w, h);
    host->setTumblerCount(id, m_count);
    if (m_indexSet)
        host->setTumblerValue(id, m_index);
    (void)minValue;
    (void)maxValue;
}

// 属性表：一行一个属性，别名用 '|' 分隔（大小写不敏感）。
const std::vector<XmlPropEntry>& XmlTumbler::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Count", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& t = static_cast<XmlTumbler&>(b);
             t.m_count = xmlprop2::ToInt(v, t.m_count);
             if (t.m_count < 1)
                 t.m_count = 1;
             if (t.m_host != nullptr)
                 t.m_host->setTumblerCount(t.m_id, t.m_count);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& t = static_cast<const XmlTumbler&>(b);
             return xmlprop2::FromInt(t.m_host != nullptr ? t.m_host->getTumblerCount(t.m_id) : t.m_count);
         }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& t = static_cast<XmlTumbler&>(b);
             t.m_index = xmlprop2::ToInt(v, t.m_index);
             t.m_indexSet = true;
             if (t.m_host != nullptr)
                 t.m_host->setTumblerValue(t.m_id, t.m_index);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& t = static_cast<const XmlTumbler&>(b);
             return xmlprop2::FromInt(t.m_host != nullptr ? t.m_host->getTumblerValue(t.m_id) : t.m_index);
         }},
    };
    return kTable;
}
