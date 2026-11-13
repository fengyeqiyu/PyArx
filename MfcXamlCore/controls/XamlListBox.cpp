// XamlListBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlListBox.h"

void XamlListBox::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addListBoxAdvanced(id, m_multiple, x, y, w, h);
        if (!m_itemsCache.empty())
            fillItems();
        if (m_selectedIndex >= 0)
            host->setCurSel(id, m_selectedIndex);
    }

std::vector<XamlProperty> XamlListBox::ownProperties() const
{
        return {
            {L"Items", XamlPropType::Items, true, true},
            {L"SelectedIndex", XamlPropType::Int, true, true},
            {L"SelectionMode", XamlPropType::Enum, true, true, L"Single|Multiple"},
        };
    }

bool XamlListBox::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));

        if (xamlprop::StrEq(key, "items"))
        {
            m_itemsCache = value;
            fillItems();
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex"))
        {
            m_selectedIndex = value.empty() ? -1 : _wtoi(value.c_str());
            m_host->setCurSel(m_id, m_selectedIndex);
            return true;
        }
        if (xamlprop::StrEq(key, "selectionmode"))
        {
            const bool multiple = (_wcsicmp(value.c_str(), L"Multiple") == 0 ||
                                   _wcsicmp(value.c_str(), L"MultiExtended") == 0 ||
                                   _wcsicmp(value.c_str(), L"multi") == 0);
            if (multiple != m_multiple)
            {
                m_multiple = multiple;
                rebuild();
            }
            return true;
        }
        return false;
    }

std::wstring XamlListBox::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            std::wstring out;
            const int n = m_host->countStrings(m_id);
            for (int i = 0; i < n; ++i)
            {
                if (i > 0) out += L"|";
                out += m_host->getString(m_id, i);
            }
            return out;
        }
        if (xamlprop::StrEq(key, "selectedindex"))
            return std::to_wstring(m_host->getCurSel(m_id));
        if (xamlprop::StrEq(key, "selectionmode"))
            return m_multiple ? L"Multiple" : L"Single";
        return {};
    }
