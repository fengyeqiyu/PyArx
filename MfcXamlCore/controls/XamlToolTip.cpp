// XamlToolTip.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlToolTip.h"

void XamlToolTip::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 12, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlToolTip::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Text", XamlPropType::String, true, true}};
    }

bool XamlToolTip::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text"))
        {
            m_host->showToolTip(m_id, value, 0);   // 0 = 不自动消失（静态展示）
            return true;
        }
        return false;
    }
