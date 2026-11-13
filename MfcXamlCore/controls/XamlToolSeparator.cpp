// XamlToolSeparator.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlToolSeparator.h"

void XamlToolSeparator::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticLine(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlToolSeparator::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Orientation", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlToolSeparator&>(b);
             c.m_orient = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlToolSeparator&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_orient;
         }},
    };
    return kTable;
}
