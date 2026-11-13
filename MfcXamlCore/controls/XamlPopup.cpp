// XamlPopup.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlPopup.h"

void XamlPopup::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 15, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlPopup::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Title", XamlPropType::String, true, true},
                {L"Content", XamlPropType::String, true, true},
                {L"Lines", XamlPropType::String, true, true}};
    }

bool XamlPopup::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "title"))
        {
            m_popTitle = value;
            return true;
        }
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "lines"))
        {
            m_popLines = xamlSplitItems(value);
            return true;
        }
        return false;
    }
