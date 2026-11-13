// XmlFrame.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlFrame.h"

void XmlFrame::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlFrame::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Padding", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlFrame&>(b);
             c.m_padding = xmlprop2::ToInt(v, c.m_padding);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlFrame&>(b).m_padding);
         }},
    };
    return kTable;
}
