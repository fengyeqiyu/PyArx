// XamlTreeCtrl.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlTreeCtrl.h"

void XamlTreeCtrl::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addTree(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlTreeCtrl::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Nodes", XamlPropType::String, true, true}};
    }

bool XamlTreeCtrl::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (!xamlprop::StrEq(key, "nodes")) return false;
        xamlextra::addNodes(m_host, m_id, value);
        return true;
    }
