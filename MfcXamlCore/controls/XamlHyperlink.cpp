// XamlHyperlink.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlHyperlink.h"

void XamlHyperlink::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addHyperlink(id, L"http://", x, y, w, h);
    }

std::vector<XamlProperty> XamlHyperlink::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Text", XamlPropType::String, true, true},
                {L"Url", XamlPropType::String, true, true}};
    }

bool XamlHyperlink::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text") || xamlprop::StrEq(key, "content"))
        {
            m_host->setText(m_id, value);
            return true;
        }
        // Url：链接地址以前根本没有属性，控件只显示一段"http://"死文字，
        // 点了没反应也没有地址可读
        if (xamlprop::StrEq(key, "url") || xamlprop::StrEq(key, "href") ||
            xamlprop::StrEq(key, "navigateurl"))
        {
            m_host->setUrl(m_id, value);
            return true;
        }
        return false;
    }

std::wstring XamlHyperlink::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "text") || xamlprop::StrEq(key, "content"))
            return m_host->getText(m_id);
        return {};
    }
