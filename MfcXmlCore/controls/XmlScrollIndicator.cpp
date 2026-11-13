// XmlScrollIndicator.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlScrollIndicator.h"

void XmlScrollIndicator::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlScrollIndicator::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Orientation", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlScrollIndicator&>(b);
             c.m_orient = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlScrollIndicator&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_orient;
         }},
        {L"Size", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlScrollIndicator&>(b);
             c.m_size = xmlprop2::ToInt(v, c.m_size);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlScrollIndicator&>(b).m_size);
         }},
        {L"Position", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlScrollIndicator&>(b);
             c.m_position = xmlprop2::ToInt(v, c.m_position);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlScrollIndicator&>(b).m_position);
         }},
    };
    return kTable;
}
