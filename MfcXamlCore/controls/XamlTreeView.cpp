// XamlTreeView.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlTreeView.h"

void XamlTreeView::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 10, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlTreeView::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Nodes", XamlPropType::String, true, true},
                {L"SelectedRow", XamlPropType::Int, true, true}};
    }

bool XamlTreeView::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "nodes"))
        {
            std::wstring cur;
            auto flush = [&]() {
                if (cur.empty())
                    return;
                // "文字@层级>" 里的 '>' 表示"有子节点"，先摘后缀再切层级
                bool kids = false;
                if (cur.back() == L'>')
                {
                    kids = true;
                    cur.pop_back();
                }
                int level = 0;
                std::wstring text = cur;
                const size_t at = cur.find_last_of(L'@');
                if (at != std::wstring::npos)
                {
                    text = cur.substr(0, at);
                    level = xamlExtraNum(cur.substr(at + 1), 0);
                }
                m_host->addTreeItem(m_id, text, level, kids);
                cur.clear();
            };
            for (wchar_t ch : value)
            {
                if (ch == L'|' || ch == L'\n' || ch == L'\r')
                    flush();
                else
                    cur.push_back(ch);
            }
            flush();
            return true;
        }
        if (xamlprop::StrEq(key, "selectedrow") || xamlprop::StrEq(key, "currentindex") ||
            xamlprop::StrEq(key, "index"))
        {
            m_host->setTreeSelectedRow(m_id, xamlExtraNum(value, -1));
            return true;
        }
        return false;
    }

std::wstring XamlTreeView::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedrow") || xamlprop::StrEq(key, "currentindex") ||
            xamlprop::StrEq(key, "index"))
            return std::to_wstring(m_host->getTreeSelectedRow(m_id));
        return {};
    }
