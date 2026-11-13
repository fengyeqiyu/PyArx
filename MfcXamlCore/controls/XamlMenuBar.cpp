// XamlMenuBar.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlMenuBar.h"

void XamlMenuBar::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 14, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlMenuBar::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Items", XamlPropType::String, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlMenuBar::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            for (const auto& s : xamlSplitItems(value))
                m_host->addMenuBarItem(m_id, s);
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "index"))
        {
            m_host->setMenuBarSelected(m_id, xamlExtraNum(value, -1));
            return true;
        }
        return false;
    }

std::wstring XamlMenuBar::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "index"))
            return std::to_wstring(m_host->getMenuBarSelected(m_id));
        return {};
    }
