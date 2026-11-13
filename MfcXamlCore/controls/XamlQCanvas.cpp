// XamlQCanvas.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlQCanvas.h"

void XamlQCanvas::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        host->addSkinBox(id, 20, x, y, w, h);
        (void)minValue;
        (void)maxValue;
    }

std::vector<XamlProperty> XamlQCanvas::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"Commands", XamlPropType::String, true, true},
                {L"Script", XamlPropType::String, true, true}};
    }

bool XamlQCanvas::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "commands") || xamlprop::StrEq(key, "script"))
        {
            m_host->setCanvasCommands(m_id, value);
            return true;
        }
        return false;
    }

std::wstring XamlQCanvas::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "commands") || xamlprop::StrEq(key, "script"))
            return m_host->getCanvasCommands(m_id);
        return {};
    }
