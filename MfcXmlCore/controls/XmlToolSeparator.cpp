// XmlToolSeparator.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlToolSeparator.h"

void XmlToolSeparator::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticLine(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlToolSeparator::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Orientation", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolSeparator&>(b);
             c.m_orient = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToolSeparator&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_orient;
         }},
    };
    return kTable;
}
