// XamlCheckBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlCheckBox.h"

void XamlCheckBox::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addCheckBox(id, L"", x, y, w, h);
    }

std::vector<XamlProperty> XamlCheckBox::ownProperties() const
{
        return {
            {L"Text", XamlPropType::String, true, true},
            {L"Checked", XamlPropType::Bool, true, true},
        };
    }

bool XamlCheckBox::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        if (xamlprop::StrEq(key, "checked"))
        {
            m_host->setCheck(m_id, xamlprop::ToBool(value));
            return true;
        }
        return false;
    }

std::wstring XamlCheckBox::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
            return m_host->getText(m_id);
        if (xamlprop::StrEq(key, "checked"))
            return m_host->getCheck(m_id) ? L"1" : L"0";
        return {};
    }
