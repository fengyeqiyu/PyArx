// XmlToolTip.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlToolTip.h"

void XmlToolTip::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 12, x, y, w, h);
    if (!m_text.empty()) host->showToolTip(id, m_text, 0);   // 0 = 不自动消失
}

const std::vector<XmlPropEntry>& XmlToolTip::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlToolTip&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->showToolTip(c.m_id, v, 0);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlToolTip&>(b).m_text; }},
    };
    return kTable;
}