// XamlTabControl.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlTabControl.h"

void XamlTabControl::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addTab(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlTabControl::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Pages", XamlPropType::String, true, true}};
    }

bool XamlTabControl::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (!xamlprop::StrEq(key, "pages")) return false;
        size_t start = 0;
        while (start <= value.size())
        {
            const size_t bar = value.find(L'|', start);
            const std::wstring item = (bar == std::wstring::npos)
                ? value.substr(start)
                : value.substr(start, bar - start);
            if (!item.empty())
                m_host->addTabItem(m_id, item);
            if (bar == std::wstring::npos)
                break;
            start = bar + 1;
        }
        return true;
    }
