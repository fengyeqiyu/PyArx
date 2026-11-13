// XamlChoice.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlChoice.h"

void XamlChoice::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addChoice(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlChoice::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Items", XamlPropType::Items, true, true}};
    }

bool XamlChoice::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            m_host->clearStrings(m_id);
            size_t start = 0;
            while (start <= value.size())
            {
                const size_t bar = value.find(L'|', start);
                const std::wstring item = (bar == std::wstring::npos)
                    ? value.substr(start)
                    : value.substr(start, bar - start);
                if (!item.empty())
                    m_host->addString(m_id, item);
                if (bar == std::wstring::npos)
                    break;
                start = bar + 1;
            }
            return true;
        }
        return false;
    }

std::wstring XamlChoice::getOwnProperty(const wchar_t* name) const
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
        return {};
    }
