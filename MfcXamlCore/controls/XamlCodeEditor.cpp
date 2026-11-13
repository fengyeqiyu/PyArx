// XamlCodeEditor.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlCodeEditor.h"

void XamlCodeEditor::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCodeEditor(id, m_text, syntaxName(), m_readOnly, x, y, w, h);
}

void XamlCodeEditor::rebuildForSyntax()
{
    if (m_host == nullptr) return;
    int x = 0, y = 0, w = 0, h = 0;
    getBounds(x, y, w, h);
    if (w <= 0 || h <= 0) return;
    m_host->addCodeEditor(m_id, m_text, syntaxName(), m_readOnly, x, y, w, h);
    pushAppearance();
}

const std::vector<XamlPropEntry>& XamlCodeEditor::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCodeEditor&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlCodeEditor&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Syntax", nullptr, XamlPropType::String, L"XAML|PYTHON",
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCodeEditor&>(b);
             const bool py = (_wcsicmp(v.c_str(), L"python") == 0) || (_wcsicmp(v.c_str(), L"py") == 0);
             if (py != c.m_python)
             {
                 c.m_python = py;
                 c.rebuildForSyntax();
             }
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlCodeEditor&>(b).syntaxName(); }},
        {L"IsReadOnly", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCodeEditor&>(b);
             c.m_readOnly = xamlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setReadOnly(c.m_id, c.m_readOnly);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlCodeEditor&>(b);
             return xamlprop2::FromBool(c.m_host != nullptr ? c.m_host->getReadOnly(c.m_id) : c.m_readOnly);
         }},
    };
    return kTable;
}