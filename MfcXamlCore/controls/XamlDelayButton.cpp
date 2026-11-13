// XamlDelayButton.cpp —— 实现（属性表版，UTF-8 with BOM）
#include "XamlDelayButton.h"

void XamlDelayButton::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addButton(id, m_text, x, y, w, h);
    if (m_checked) host->setCheck(id, true);
}

// 属性表：一行一个属性（名称 / 别名 / 类型 / 枚举 / 读 / 写）
const std::vector<XamlPropEntry>& XamlDelayButton::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDelayButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDelayButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Delay", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDelayButton&>(b);
             c.m_delay = xamlprop2::ToInt(v, c.m_delay);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlDelayButton&>(b).m_delay);
         }},
        {L"Checked", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlDelayButton&>(b);
             c.m_checked = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setCheck(c.m_id, c.m_checked);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlDelayButton&>(b);
             return xamlprop2::FromBool(c.m_host != nullptr ? c.m_host->getCheck(c.m_id) : c.m_checked);
         }},
    };
    return kTable;
}
