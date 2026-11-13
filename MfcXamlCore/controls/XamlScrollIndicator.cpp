// XamlScrollIndicator.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlScrollIndicator.h"

void XamlScrollIndicator::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlScrollIndicator::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Orientation", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlScrollIndicator&>(b);
             c.m_orient = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlScrollIndicator&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_orient;
         }},
        {L"Size", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlScrollIndicator&>(b);
             c.m_size = xamlprop2::ToInt(v, c.m_size);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlScrollIndicator&>(b).m_size);
         }},
        {L"Position", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlScrollIndicator&>(b);
             c.m_position = xamlprop2::ToInt(v, c.m_position);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlScrollIndicator&>(b).m_position);
         }},
    };
    return kTable;
}
