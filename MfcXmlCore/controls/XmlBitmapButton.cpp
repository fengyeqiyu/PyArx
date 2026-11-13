// XmlBitmapButton.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlBitmapButton.h"

void XmlBitmapButton::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addBitmapButton(id, m_text, x, y, w, h);
    if (!m_image.empty()) host->setImage(id, m_image);
}

const std::vector<XmlPropEntry>& XmlBitmapButton::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlBitmapButton&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlBitmapButton&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Image", nullptr, XmlPropType::Image, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlBitmapButton&>(b);
             c.m_image = v;
             if (c.m_host != nullptr) c.m_host->setImage(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlBitmapButton&>(b).m_image; }},
    };
    return kTable;
}