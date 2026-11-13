// XamlToolTip.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlToolTip.h"

void XamlToolTip::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 12, x, y, w, h);
    if (!m_text.empty()) host->showToolTip(id, m_text, 0);   // 0 = 不自动消失
}

const std::vector<XamlPropEntry>& XamlToolTip::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlToolTip&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->showToolTip(c.m_id, v, 0);
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlToolTip&>(b).m_text; }},
    };
    return kTable;
}