// XamlFrame.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlFrame.h"

void XamlFrame::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlFrame::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Padding", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlFrame&>(b);
             c.m_padding = xamlprop2::ToInt(v, c.m_padding);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlFrame&>(b).m_padding);
         }},
    };
    return kTable;
}
