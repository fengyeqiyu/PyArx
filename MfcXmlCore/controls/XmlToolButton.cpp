// XmlToolButton.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlToolButton.h"

void XmlToolButton::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addButton(id, m_text, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlToolButton::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToolButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"IconSource", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolButton&>(b);
             c.m_icon = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToolButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_icon;
         }},
        {L"Display", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolButton&>(b);
             c.m_display = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlToolButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_display;
         }},
    };
    return kTable;
}
