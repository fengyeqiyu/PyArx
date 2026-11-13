// XamlGrid.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlGrid.h"

void XamlGrid::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addGrid(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlGrid::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Columns", XamlPropType::String, true, true},
                {L"Rows", XamlPropType::String, true, true}};
    }

bool XamlGrid::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "columns"))
        {
            xamlextra::addColumns(m_host, m_id, value);
            return true;
        }
        if (xamlprop::StrEq(key, "rows"))
        {
            xamlextra::addRows(m_host, m_id, value);
            return true;
        }
        return false;
    }
