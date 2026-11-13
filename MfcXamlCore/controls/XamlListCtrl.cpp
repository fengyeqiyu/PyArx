// XamlListCtrl.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlListCtrl.h"

void XamlListCtrl::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addListCtrl(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlListCtrl::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Columns", XamlPropType::String, true, true},
                {L"Rows", XamlPropType::String, true, true}};
    }

bool XamlListCtrl::setOwnProperty(const wchar_t* name, const std::wstring& value)
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
