// XamlToggleButton.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlToggleButton.h"

void XamlToggleButton::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addToggle(id, L"", x, y, w, h);
    }

std::vector<XamlProperty> XamlToggleButton::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Content", XamlPropType::String, true, true},
                {L"Checked", XamlPropType::Bool, true, true}};
    }

bool XamlToggleButton::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        if (xamlprop::StrEq(key, "checked"))
        {
            m_host->setCheck(m_id, value == L"true" || value == L"1");
            return true;
        }
        return false;
    }

std::wstring XamlToggleButton::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
            return m_host->getText(m_id);
        if (xamlprop::StrEq(key, "checked"))
            return m_host->getCheck(m_id) ? L"true" : L"false";
        return {};
    }
