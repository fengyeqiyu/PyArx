// XamlDrawer.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlDrawer.h"

void XamlDrawer::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlDrawer::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Edge", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDrawer&>(b);
             c.m_edge = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDrawer&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_edge;
         }},
        {L"Modal", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDrawer&>(b);
             c.m_modal = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_modal);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDrawer&>(b);
             return xamlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_modal);
         }},
        {L"Position", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDrawer&>(b);
             c.m_position = xamlprop2::ToInt(v, c.m_position);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlDrawer&>(b).m_position);
         }},
    };
    return kTable;
}
