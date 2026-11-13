// XmlSwitch.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlSwitch.h"

void XmlSwitch::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 1, x, y, w, h);
    if (m_checked) host->setSwitchValue(id, true);
}

const std::vector<XmlPropEntry>& XmlSwitch::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Checked", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlSwitch&>(b);
             c.m_checked = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setSwitchValue(c.m_id, c.m_checked);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlSwitch&>(b);
             const bool on = c.m_host != nullptr ? c.m_host->getSwitchValue(c.m_id) : c.m_checked;
             return xmlprop2::FromBool(on);
         }},
    };
    return kTable;
}