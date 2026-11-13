// XamlMenu.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlMenu.h"

void XamlMenu::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 13, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlMenu::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Items", XamlPropType::String, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlMenu::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            m_host->setMenuItems(m_id, xamlSplitItems(value));
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex"))
            return false;   // 只读：点击后由 getMenuSelected 回读
        return false;
    }

std::wstring XamlMenu::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex"))
            return std::to_wstring(m_host->getMenuSelected(m_id));
        return {};
    }
