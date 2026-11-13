// XamlBitmapButton.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlBitmapButton.h"

void XamlBitmapButton::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addBitmapButton(id, m_text, x, y, w, h);
    if (!m_image.empty()) host->setImage(id, m_image);
}

const std::vector<XamlPropEntry>& XamlBitmapButton::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlBitmapButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlBitmapButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Image", nullptr, XamlPropType::Image, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlBitmapButton&>(b);
             c.m_image = v;
             if (c.m_host != nullptr) c.m_host->setImage(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlBitmapButton&>(b).m_image; }},
    };
    return kTable;
}