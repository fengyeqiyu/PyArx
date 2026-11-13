// XmlDialog.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XmlDialog.h"

void XmlDialog::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XmlPropEntry>& XmlDialog::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Title", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDialog&>(b);
             c.m_title = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDialog&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_title;
         }},
        {L"Modal", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDialog&>(b);
             c.m_modal = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_modal);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDialog&>(b);
             return xmlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_modal);
         }},
        {L"StandardButtons", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlDialog&>(b);
             c.m_buttons = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlDialog&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_buttons;
         }},
    };
    return kTable;
}
