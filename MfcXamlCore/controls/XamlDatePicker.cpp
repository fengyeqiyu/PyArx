// XamlDatePicker.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlDatePicker.h"

void XamlDatePicker::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addDatePicker(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlDatePicker::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Value", XamlPropType::String, true, true}};
    }

bool XamlDatePicker::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "value"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        return false;
    }

std::wstring XamlDatePicker::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "value"))
            return m_host->getText(m_id);
        return {};
    }
