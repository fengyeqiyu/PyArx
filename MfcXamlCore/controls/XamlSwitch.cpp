// XamlSwitch.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlSwitch.h"

void XamlSwitch::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 1, x, y, w, h);
    if (m_checked) host->setSwitchValue(id, true);
}

const std::vector<XamlPropEntry>& XamlSwitch::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Checked", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlSwitch&>(b);
             c.m_checked = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setSwitchValue(c.m_id, c.m_checked);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlSwitch&>(b);
             const bool on = c.m_host != nullptr ? c.m_host->getSwitchValue(c.m_id) : c.m_checked;
             return xamlprop2::FromBool(on);
         }},
    };
    return kTable;
}