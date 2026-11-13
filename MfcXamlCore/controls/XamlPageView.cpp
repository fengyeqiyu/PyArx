// XamlPageView.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlPageView.h"

void XamlPageView::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 17, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlPageView::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlPageView::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex"))
        {
            m_host->setPageIndex(m_id, xamlExtraNum(value, 0));
            return true;
        }
        return false;
    }

std::wstring XamlPageView::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "selectedindex"))
            return std::to_wstring(m_host->getPageIndex(m_id));
        if (xamlprop::StrEq(key, "pagecount"))
            return std::to_wstring(m_host->getPageCount(m_id));
        return {};
    }
