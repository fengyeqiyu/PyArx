// XamlItemView.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlItemView.h"

void XamlItemView::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlItemView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Model", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlItemView&>(b);
             c.m_model = v;
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return static_cast<const XamlItemView&>(b).m_model;
         }},
        {L"ItemHeight", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlItemView&>(b);
             c.m_itemH = xamlprop2::ToInt(v, c.m_itemH);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlItemView&>(b).m_itemH);
         }},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlItemView&>(b);
             c.m_selectedIndex = xamlprop2::ToInt(v, c.m_selectedIndex);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlItemView&>(b).m_selectedIndex);
         }},
    };
    return kTable;
}
