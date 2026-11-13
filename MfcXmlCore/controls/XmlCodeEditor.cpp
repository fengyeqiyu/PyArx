// XmlCodeEditor.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlCodeEditor.h"

void XmlCodeEditor::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCodeEditor(id, m_text, syntaxName(), m_readOnly, x, y, w, h);
}

void XmlCodeEditor::rebuildForSyntax()
{
    if (m_host == nullptr) return;
    int x = 0, y = 0, w = 0, h = 0;
    getBounds(x, y, w, h);
    if (w <= 0 || h <= 0) return;
    m_host->addCodeEditor(m_id, m_text, syntaxName(), m_readOnly, x, y, w, h);
    pushAppearance();
}

const std::vector<XmlPropEntry>& XmlCodeEditor::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlCodeEditor&>(b);
             c.m_text = v;
             if (c.m_host != nullptr) c.m_host->setText(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlCodeEditor&>(b);
             return c.m_host != nullptr ? c.m_host->getText(c.m_id) : c.m_text;
         }},
        {L"Syntax", nullptr, XmlPropType::String, L"XML|PYTHON",
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlCodeEditor&>(b);
             const bool py = (_wcsicmp(v.c_str(), L"python") == 0) || (_wcsicmp(v.c_str(), L"py") == 0);
             if (py != c.m_python)
             {
                 c.m_python = py;
                 c.rebuildForSyntax();
             }
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlCodeEditor&>(b).syntaxName(); }},
        {L"IsReadOnly", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlCodeEditor&>(b);
             c.m_readOnly = xmlprop2::ToBool(v);
             if (c.m_host != nullptr) c.m_host->setReadOnly(c.m_id, c.m_readOnly);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlCodeEditor&>(b);
             return xmlprop2::FromBool(c.m_host != nullptr ? c.m_host->getReadOnly(c.m_id) : c.m_readOnly);
         }},
    };
    return kTable;
}