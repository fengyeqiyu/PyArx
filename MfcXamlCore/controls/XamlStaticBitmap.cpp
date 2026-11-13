// XamlStaticBitmap.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlStaticBitmap.h"

void XamlStaticBitmap::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticBitmap(id, x, y, w, h);
    if (!m_image.empty()) host->setImage(id, m_image);
}

const std::vector<XamlPropEntry>& XamlStaticBitmap::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Image", nullptr, XamlPropType::Image, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlStaticBitmap&>(b);
             c.m_image = v;
             if (c.m_host != nullptr) c.m_host->setImage(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlStaticBitmap&>(b).m_image; }},
    };
    return kTable;
}