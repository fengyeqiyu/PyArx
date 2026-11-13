// XamlTextBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XamlTextBox.h"

void XamlTextBox::createNative(IXamlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
    host->addEditAdvanced(id, m_text, m_multiline, m_readOnly, m_maxLength,
                          m_placeholder, x, y, w, h);
}

// 属性表：一个属性一行，别名用 '|' 分隔，读写直接落到本类成员/宿主。
// Text/Content/Caption 是同一个东西（历史上三种写法都用过），这里归一。
const std::vector<XamlPropEntry>& XamlTextBox::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Text", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTextBox&>(b).SetTextProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTextBox&>(b).GetTextProp(); }},
        {L"IsReadOnly", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTextBox&>(b).SetReadOnlyProp(v); },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromBool(static_cast<const XamlTextBox&>(b).m_readOnly); }},
        {L"Multiline", nullptr, XamlPropType::Bool, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTextBox&>(b).SetMultilineProp(v); },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromBool(static_cast<const XamlTextBox&>(b).m_multiline); }},
        {L"MaxLength", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTextBox&>(b).SetMaxLengthProp(v); },
         [](const XamlMfcControlBase& b) {
             return xamlprop2::FromInt(static_cast<const XamlTextBox&>(b).m_maxLength); }},
        {L"Placeholder", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTextBox&>(b).SetPlaceholderProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTextBox&>(b).m_placeholder; }},
    };
    return kTable;
}

bool XamlTextBox::SetTextProp(const std::wstring& v)
{
    m_text = v;
    if (m_host != nullptr)
        m_host->setText(m_id, v);
    return true;
}

std::wstring XamlTextBox::GetTextProp() const
{
    return m_host != nullptr ? m_host->getText(m_id) : m_text;
}

bool XamlTextBox::SetReadOnlyProp(const std::wstring& v)
{
    const bool v2 = xamlprop2::ToBool(v);
    if (v2 != m_readOnly)
    {
        m_readOnly = v2;
        if (m_host != nullptr)
            m_host->setReadOnly(m_id, m_readOnly);
    }
    return true;
}

bool XamlTextBox::SetMultilineProp(const std::wstring& v)
{
    const bool v2 = xamlprop2::ToBool(v);
    if (v2 != m_multiline)
    {
        m_multiline = v2;
        rebuild();
    }
    return true;
}

bool XamlTextBox::SetMaxLengthProp(const std::wstring& v)
{
    const int v2 = xamlprop2::ToInt(v, 0);
    if (v2 != m_maxLength)
    {
        m_maxLength = v2;
        if (m_host != nullptr)
            m_host->setLimitText(m_id, v2);
    }
    return true;
}

bool XamlTextBox::SetPlaceholderProp(const std::wstring& v)
{
    if (v != m_placeholder)
    {
        m_placeholder = v;
        if (m_host != nullptr)
            m_host->setCueBanner(m_id, v);
    }
    return true;
}
