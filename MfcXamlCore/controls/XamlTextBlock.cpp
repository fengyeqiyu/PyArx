// XamlTextBlock.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlTextBlock.h"

void XamlTextBlock::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addLabel(id, L"", x, y, w, h);
    }

std::vector<XamlProperty> XamlTextBlock::ownProperties() const
{
        return {{L"Text", XamlPropType::String, true, true}};
    }

bool XamlTextBlock::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        return false;
    }

std::wstring XamlTextBlock::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
            return m_host->getText(m_id);
        return {};
    }
