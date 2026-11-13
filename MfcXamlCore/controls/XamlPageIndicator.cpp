// XamlPageIndicator.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlPageIndicator.h"

void XamlPageIndicator::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addSkinBox(id, 5, x, y, w, h);
        host->setPageCount(id, 3);
    }

std::vector<XamlProperty> XamlPageIndicator::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Count", XamlPropType::Int, true, true},
                {L"SelectedIndex", XamlPropType::Int, true, true}};
    }

bool XamlPageIndicator::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        int n = 0;
        try { n = std::stoi(value); } catch (...) { return false; }
        if (xamlprop::StrEq(key, "count"))
        {
            m_host->setPageCount(m_id, n);
            return true;
        }
        if (xamlprop::StrEq(key, "selectedindex"))
        {
            m_host->setPageIndex(m_id, n);
            return true;
        }
        return false;
    }

std::wstring XamlPageIndicator::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "count"))
            return std::to_wstring(m_host->getPageCount(m_id));
        if (xamlprop::StrEq(key, "selectedindex"))
            return std::to_wstring(m_host->getPageIndex(m_id));
        return {};
    }
