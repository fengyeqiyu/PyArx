// XamlButton.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlButton.h"

void XamlButton::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addButton(id, L"", x, y, w, h);
    }

std::vector<XamlProperty> XamlButton::ownProperties() const
{
        return {{L"Content", XamlPropType::String, true, true}};
    }

bool XamlButton::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        return false;
    }

std::wstring XamlButton::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
            return m_host->getText(m_id);
        return {};
    }
