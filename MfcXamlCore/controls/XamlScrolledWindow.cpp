// XamlScrolledWindow.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlScrolledWindow.h"

void XamlScrolledWindow::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addScrolledWindow(id, x, y, w, h);
    }

std::vector<XamlProperty> XamlScrolledWindow::ownProperties() const
{
        return {{L"Name", XamlPropType::String, true, true},
                {L"ScrollMin", XamlPropType::Int, true, true},
                {L"ScrollMax", XamlPropType::Int, true, true}};
    }

bool XamlScrolledWindow::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "scrollmin") || xamlprop::StrEq(key, "minimum"))
        {
            m_minValue = _wtoi(value.c_str());
            applyRange();
            return true;
        }
        if (xamlprop::StrEq(key, "scrollmax") || xamlprop::StrEq(key, "maximum"))
        {
            m_maxValue = _wtoi(value.c_str());
            applyRange();
            return true;
        }
        return false;
    }
