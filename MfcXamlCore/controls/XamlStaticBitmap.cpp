// XamlStaticBitmap.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlStaticBitmap.h"

void XamlStaticBitmap::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addStaticBitmap(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlStaticBitmap::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Image", XamlPropType::String, true, true}};
    }

bool XamlStaticBitmap::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "image") || xamlprop::StrEq(key, "src") ||
            xamlprop::StrEq(key, "source"))
        {
            m_host->setImage(m_id, value);
            return true;
        }
        return false;
    }
