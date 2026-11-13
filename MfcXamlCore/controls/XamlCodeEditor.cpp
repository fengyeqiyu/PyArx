// XamlCodeEditor.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlCodeEditor.h"

void XamlCodeEditor::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
        host->addCodeEditor(id, m_text, syntaxName(), m_readOnly, x, y, w, h);
    }

std::vector<XamlProperty> XamlCodeEditor::ownProperties() const
{
        return {
            {L"Content", XamlPropType::String, true, true},
            {L"Text", XamlPropType::String, true, true},
            {L"Syntax", XamlPropType::String, true, true},
            {L"IsReadOnly", XamlPropType::Bool, true, true},
        };
    }

bool XamlCodeEditor::setOwnProperty(const wchar_t* name, const std::wstring& value)
{
        if (m_host == nullptr) return false;
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));

        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
        {
            m_text = value;
            m_host->setText(m_id, value);
            return true;
        }
        if (xamlprop::StrEq(key, "syntax"))
        {
            const bool py = (_wcsicmp(value.c_str(), L"python") == 0) ||
                            (_wcsicmp(value.c_str(), L"py") == 0);
            const bool next = py;
            if (next != m_python)
            {
                m_python = next;
                // 语法是创建期属性：按当前矩形重建一次
                int x = 0, y = 0, w = 0, h = 0;
                getBounds(x, y, w, h);
                if (w > 0 && h > 0)
                {
                    m_host->addCodeEditor(m_id, m_text, syntaxName(), m_readOnly, x, y, w, h);
                    pushAppearance();
                }
            }
            return true;
        }
        if (xamlprop::StrEq(key, "isreadonly") || xamlprop::StrEq(key, "readonly"))
        {
            m_readOnly = xamlprop::ToBool(value);
            m_host->setReadOnly(m_id, m_readOnly);
            return true;
        }
        return false;
    }

std::wstring XamlCodeEditor::getOwnProperty(const wchar_t* name) const
{
        if (m_host == nullptr) return {};
        const std::string key = xamlprop::Lower(xamlprop::WideToUtf8(name ? name : L""));
        if (xamlprop::StrEq(key, "content") || xamlprop::StrEq(key, "text"))
            return m_host->getText(m_id);
        if (xamlprop::StrEq(key, "syntax"))
            return syntaxName();
        if (xamlprop::StrEq(key, "isreadonly") || xamlprop::StrEq(key, "readonly"))
            return m_readOnly ? L"1" : L"0";
        return {};
    }
