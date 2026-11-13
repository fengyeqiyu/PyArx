// XamlListView.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlListView.h"

void XamlListView::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 16, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlListView::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Items", XamlPropType::String, true, true},
                {L"ItemHeight", XamlPropType::Int, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlListView::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "items"))
        {
            m_host->clearListItems(m_id);
            std::wstring cur;
            for (wchar_t ch : value)
            {
                if (ch == L';' || ch == L'\n' || ch == L'\r')
                {
                    if (!cur.empty())
                        m_host->addListItem(m_id, cur);
                    cur.clear();
                }
                else
                {
                    cur.push_back(ch);
                }
            }
            if (!cur.empty())
                m_host->addListItem(m_id, cur);
            return true;
        }
        if (xamlprop::StrEq(key, "itemheight"))
        {
            // 行高先记到宿主：没有专门 API 时按选中接口旁的默认 44 走
            m_itemH = xamlExtraNum(value, 44);
            if (m_itemH < 20) m_itemH = 20;
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "index"))
        {
            m_host->setListSelected(m_id, xamlExtraNum(value, -1));
            return true;
        }
        return false;
    }

std::wstring XamlListView::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex") || xamlprop::StrEq(key, "index"))
            return std::to_wstring(m_host->getListSelected(m_id));
        if (xamlprop::StrEq(key, "itemcount"))
            return std::to_wstring(m_host->getListItemCount(m_id));
        return {};
    }
