// XmlMenuSeparator.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlMenuSeparator.h"

void XmlMenuSeparator::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticLine(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlMenuSeparator::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Orientation", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlMenuSeparator&>(b);
             c.m_orient = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlMenuSeparator&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_orient;
         }},
    };
    return kTable;
}
