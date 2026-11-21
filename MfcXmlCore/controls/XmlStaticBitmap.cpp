// XmlStaticBitmap.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlStaticBitmap.h"

void XmlStaticBitmap::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addStaticBitmap(id, x, y, w, h);
    if (!m_objectFit.empty()) host->setImageFit(id, m_objectFit);
    if (!m_objectPos.empty()) host->setImagePosition(id, m_objectPos);
    if (!m_image.empty()) host->setImage(id, m_image);
}

const std::vector<XmlPropEntry>& XmlStaticBitmap::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Image", nullptr, XmlPropType::Image, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlStaticBitmap&>(b);
             c.m_image = v;
             if (c.m_host != nullptr) c.m_host->setImage(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlStaticBitmap&>(b).m_image; }},
        {L"ObjectFit", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlStaticBitmap&>(b);
             c.m_objectFit = v;
             if (c.m_host != nullptr) c.m_host->setImageFit(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlStaticBitmap&>(b).m_objectFit; }},
        {L"ObjectPosition", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlStaticBitmap&>(b);
             c.m_objectPos = v;
             if (c.m_host != nullptr) c.m_host->setImagePosition(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlStaticBitmap&>(b).m_objectPos; }},
    };
    return kTable;
}