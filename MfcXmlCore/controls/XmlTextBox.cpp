// XmlTextBox.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）
#include "XmlTextBox.h"

void XmlTextBox::createNative(IXmlMfcHost* host, int id, int /*min*/, int /*max*/, int x, int y, int w, int h)
{
    host->addEditAdvanced(id, m_text, m_multiline, m_readOnly, m_maxLength,
                          m_placeholder, x, y, w, h);
    applyPassword();
}

// 属性表：一个属性一行，别名用 '|' 分隔，读写直接落到本类成员/宿主。
// Text/Content/Caption 是同一个东西（历史上三种写法都用过），这里归一。
const std::vector<XmlPropEntry>& XmlTextBox::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Text", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetTextProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTextBox&>(b).GetTextProp(); }},
        // 别名必须放 Aliases 字段（'|' 分隔）；塞进 Name 会让逐字比对把整串当名字，永不命中
        {L"IsReadOnly", L"ReadOnly|readonly", XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetReadOnlyProp(v); },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromBool(static_cast<const XmlTextBox&>(b).m_readOnly); }},
        {L"Multiline", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetMultilineProp(v); },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromBool(static_cast<const XmlTextBox&>(b).m_multiline); }},
        {L"MaxLength", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetMaxLengthProp(v); },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromInt(static_cast<const XmlTextBox&>(b).m_maxLength); }},
        {L"Placeholder", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetPlaceholderProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTextBox&>(b).m_placeholder; }},
        {L"Password", nullptr, XmlPropType::Bool, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTextBox&>(b).SetPasswordProp(v); },
         [](const XmlMfcControlBase& b) {
             return xmlprop2::FromBool(static_cast<const XmlTextBox&>(b).m_password); }},
    };
    return kTable;
}

bool XmlTextBox::SetPasswordProp(const std::wstring& v)
{
    const bool v2 = xmlprop2::ToBool(v);
    if (v2 != m_password)
    {
        m_password = v2;
        applyPassword();
    }
    return true;
}

bool XmlTextBox::SetTextProp(const std::wstring& v)
{
    m_text = v;
    if (m_host != nullptr)
        m_host->setText(m_id, v);
    return true;
}

std::wstring XmlTextBox::GetTextProp() const
{
    return m_host != nullptr ? m_host->getText(m_id) : m_text;
}

bool XmlTextBox::SetReadOnlyProp(const std::wstring& v)
{
    const bool v2 = xmlprop2::ToBool(v);
    if (v2 != m_readOnly)
    {
        m_readOnly = v2;
        if (m_host != nullptr)
            m_host->setReadOnly(m_id, m_readOnly);
    }
    return true;
}

bool XmlTextBox::SetMultilineProp(const std::wstring& v)
{
    const bool v2 = xmlprop2::ToBool(v);
    if (v2 != m_multiline)
    {
        m_multiline = v2;
        rebuild();
    }
    return true;
}

bool XmlTextBox::SetMaxLengthProp(const std::wstring& v)
{
    const int v2 = xmlprop2::ToInt(v, 0);
    if (v2 != m_maxLength)
    {
        m_maxLength = v2;
        if (m_host != nullptr)
            m_host->setLimitText(m_id, v2);
    }
    return true;
}

bool XmlTextBox::SetPlaceholderProp(const std::wstring& v)
{
    if (v != m_placeholder)
    {
        m_placeholder = v;
        if (m_host != nullptr)
            m_host->setCueBanner(m_id, v);
    }
    return true;
}
