// XamlBitmapButton.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlBitmapButton.h"

void XamlBitmapButton::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addBitmapButton(id, L"", x, y, w, h);
    }

std::vector<XamlProperty> XamlBitmapButton::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Content", XamlPropType::String, true, true},
                {L"Image", XamlPropType::String, true, true}};
    }

bool XamlBitmapButton::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        if (xamlprop::StrEq(key, "image") || xamlprop::StrEq(key, "src") ||
            xamlprop::StrEq(key, "source"))
        {
            m_host->setImage(m_id, value);
            return true;
        }
        return false;
    }
