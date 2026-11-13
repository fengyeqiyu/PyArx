// XamlRichText.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlRichText.h"

bool XamlRichText::setProperty(const wchar_t* name, const std::wstring& value)
{
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "fontsize") || xamlprop::StrEq(key, "bold") ||
            xamlprop::StrEq(key, "italic"))
        {
            setOwnProperty(name, value);
            return true;
        }
        return XamlMfcControlBase::setProperty(name, value);
    }

void XamlRichText::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addRichText(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlRichText::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Text", XamlPropType::String, true, true},
                {L"FontSize", XamlPropType::Int, true, true},
                {L"Bold", XamlPropType::Bool, true, true},
                {L"Italic", XamlPropType::Bool, true, true},
                {L"Foreground", XamlPropType::String, true, true}};
    }

bool XamlRichText::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        // 字号/粗体/斜体/颜色：原生 RichEdit 一直支持，只是以前没入口，
        // Python 侧只能拿到一段样式固定的纯文本
        if (xamlprop::StrEq(key, "fontsize") || xamlprop::StrEq(key, "bold") ||
            xamlprop::StrEq(key, "italic") || xamlprop::StrEq(key, "foreground"))
        {
            if (xamlprop::StrEq(key, "fontsize"))
                m_fontPt = _wtoi(value.c_str());
            else if (xamlprop::StrEq(key, "bold"))
                m_bold = (value == L"true" || value == L"1");
            else if (xamlprop::StrEq(key, "italic"))
                m_italic = (value == L"true" || value == L"1");
            else
            {
                COLORREF c = 0;
                if (xamlapp::ParseColor(value, c))
                    m_color = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
            }
            m_host->setRichFormat(m_id, m_fontPt, m_bold, m_italic, m_color);
            return true;
        }
        return false;
    }
