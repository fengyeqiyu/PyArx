// XamlDialog.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlDialog.h"

void XamlDialog::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addPanel(id, x, y, w, h);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlDialog::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Title", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDialog&>(b);
             c.m_title = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDialog&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_title;
         }},
        {L"Modal", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDialog&>(b);
             c.m_modal = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_modal);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDialog&>(b);
             return xamlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_modal);
         }},
        {L"StandardButtons", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDialog&>(b);
             c.m_buttons = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDialog&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_buttons;
         }},
    };
    return kTable;
}
