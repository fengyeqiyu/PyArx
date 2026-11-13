// XamlSwitch.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlSwitch.h"

void XamlSwitch::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addSkinBox(id, 1, x, y, w, h);
    }

std::vector<XamlProperty> XamlSwitch::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Checked", XamlPropType::Bool, true, true}};
    }

bool XamlSwitch::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "checked"))
        {
            const std::string v = xamlprop::Lower(xamlprop::WideToUtf8(value));
            const bool on = xamlprop::StrEq(v, "true") || xamlprop::StrEq(v, "1");
            m_host->setSwitchValue(m_id, on);
            return true;
        }
        return false;
    }

std::wstring XamlSwitch::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "checked"))
            return m_host->getSwitchValue(m_id) ? L"true" : L"false";
        return {};
    }
