// XamlRichText.cpp —— 属性表实现（UTF-8 with BOM）
// FontSize / Foreground 归基类统一外观（一个名字只能出现一次）；
// 这里通过 onAppearanceChanged 把外观字段转成 RichEdit 的 CHARFORMAT。
#include "XamlRichText.h"

void XamlRichText::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addRichText(id, x, y, w, h);
    if (!m_text.empty()) host->setText(id, m_text);
    applyFormat();
}

void XamlRichText::applyFormat()
{
    if (m_host != nullptr)
        m_host->setRichFormat(m_id, m_fontPt, m_bold, m_italic, m_color);
}

void XamlRichText::onAppearanceChanged()
{
    const xamlapp::XamlAppearance& ap = appearance();
    if (ap.fontSize > 0)
        m_fontPt = ap.fontSize;
    if (!ap.foreColor.empty())
    {
        COLORREF rgb = 0;
        if (xamlapp::ParseColor(ap.foreColor, rgb))
            m_color = (GetRValue(rgb) << 16) | (GetGValue(rgb) << 8) | GetBValue(rgb);
    }
    applyFormat();
}

const std::vector<XamlPropEntry>& XamlRichText::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlRichText&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlRichText&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Bold", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlRichText&>(b);
             c.m_bold = xamlprop2::ToBool(v);
             c.applyFormat();
             return true;
         },
         [](const XamlMfcControlBase& b) { return xamlprop2::FromBool(static_cast<const XamlRichText&>(b).m_bold); }},
        {L"Italic", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlRichText&>(b);
             c.m_italic = xamlprop2::ToBool(v);
             c.applyFormat();
             return true;
         },
         [](const XamlMfcControlBase& b) { return xamlprop2::FromBool(static_cast<const XamlRichText&>(b).m_italic); }},
    };
    return kTable;
}