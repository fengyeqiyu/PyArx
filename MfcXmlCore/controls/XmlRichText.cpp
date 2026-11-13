// XmlRichText.cpp —— 属性表实现（UTF-8 with BOM）
// FontSize / Foreground 归基类统一外观（一个名字只能出现一次）；
// 这里通过 onAppearanceChanged 把外观字段转成 RichEdit 的 CHARFORMAT。
#include "XmlRichText.h"

void XmlRichText::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addRichText(id, x, y, w, h);
    if (!m_text.empty()) host->setText(id, m_text);
    applyFormat();
}

void XmlRichText::applyFormat()
{
    if (m_host != nullptr)
        m_host->setRichFormat(m_id, m_fontPt, m_bold, m_italic, m_color);
}

void XmlRichText::onAppearanceChanged()
{
    const xmlapp::XmlAppearance& ap = appearance();
    if (ap.fontSize > 0)
        m_fontPt = ap.fontSize;
    if (!ap.foreColor.empty())
    {
        COLORREF rgb = 0;
        if (xmlapp::ParseColor(ap.foreColor, rgb))
            m_color = (GetRValue(rgb) << 16) | (GetGValue(rgb) << 8) | GetBValue(rgb);
    }
    applyFormat();
}

const std::vector<XmlPropEntry>& XmlRichText::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlRichText&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlRichText&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Bold", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlRichText&>(b);
             c.m_bold = xmlprop2::ToBool(v);
             c.applyFormat();
             return true;
         },
         [](const XmlMfcControlBase& b) { return xmlprop2::FromBool(static_cast<const XmlRichText&>(b).m_bold); }},
        {L"Italic", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlRichText&>(b);
             c.m_italic = xmlprop2::ToBool(v);
             c.applyFormat();
             return true;
         },
         [](const XmlMfcControlBase& b) { return xmlprop2::FromBool(static_cast<const XmlRichText&>(b).m_italic); }},
    };
    return kTable;
}