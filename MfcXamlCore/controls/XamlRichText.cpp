// XamlRichText.cpp —— 属性表实现（UTF-8 with BOM）
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
        {L"FontSize", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlRichText&>(b);
             c.m_fontPt = xamlprop2::ToInt(v, 9);
             c.applyFormat();
             return true;
         },
         [](const XamlMfcControlBase& b) { return std::to_wstring(static_cast<const XamlRichText&>(b).m_fontPt); }},
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
        {L"Foreground", nullptr, XamlPropType::Color, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlRichText&>(b);
             COLORREF rgb = 0;
             if (xamlapp::ParseColor(v, rgb))
             {
                 c.m_color = (GetRValue(rgb) << 16) | (GetGValue(rgb) << 8) | GetBValue(rgb);
                 c.applyFormat();
             }
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const unsigned long rgb = static_cast<const XamlRichText&>(b).m_color;
             wchar_t buf[16];
             _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"#%02lX%02lX%02lX",
                          (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
             return std::wstring(buf);
         }},
    };
    return kTable;
}