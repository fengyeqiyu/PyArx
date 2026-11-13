// XmlItemView.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlItemView.h"

void XmlItemView::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlItemView::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Model", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlItemView&>(b);
             c.m_model = v;
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return static_cast<const XmlItemView&>(b).m_model;
         }},
        {L"ItemHeight", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlItemView&>(b);
             c.m_itemH = xmlprop2::ToInt(v, c.m_itemH);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlItemView&>(b).m_itemH);
         }},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlItemView&>(b);
             c.m_selectedIndex = xmlprop2::ToInt(v, c.m_selectedIndex);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlItemView&>(b).m_selectedIndex);
         }},
    };
    return kTable;
}
