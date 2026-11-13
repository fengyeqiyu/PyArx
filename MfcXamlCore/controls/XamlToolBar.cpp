// XamlToolBar.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlToolBar.h"

void XamlToolBar::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 11, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlToolBar::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Items", XamlPropType::String, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlToolBar::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            std::wstring cur;
            for (wchar_t ch : value)
            {
                if (ch == L'|' || ch == L'\n' || ch == L'\r')
                {
                    if (!cur.empty())
                        m_host->addToolBarItem(m_id, cur);
                    cur.clear();
                }
                else
                {
                    cur.push_back(ch);
                }
            }
            if (!cur.empty())
                m_host->addToolBarItem(m_id, cur);
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "currentindex") ||
            xamlprop::StrEq(key, "index"))
        {
            m_host->setToolBarSelected(m_id, xamlExtraNum(value, -1));
            return true;
        }
        return false;
    }

std::wstring XamlToolBar::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "currentindex") ||
            xamlprop::StrEq(key, "index"))
            return std::to_wstring(m_host->getToolBarSelected(m_id));
        return {};
    }
