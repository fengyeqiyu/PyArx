// XmlDrawer.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlDrawer.h"

void XmlDrawer::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlDrawer::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Edge", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDrawer&>(b);
             c.m_edge = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDrawer&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_edge;
         }},
        {L"Modal", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDrawer&>(b);
             c.m_modal = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_modal);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDrawer&>(b);
             return xmlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_modal);
         }},
        {L"Position", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDrawer&>(b);
             c.m_position = xmlprop2::ToInt(v, c.m_position);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlDrawer&>(b).m_position);
         }},
    };
    return kTable;
}
