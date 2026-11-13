// XmlTableView.cpp —— 实现（拆分自 XmlExtraControls.h，UTF-8 with BOM）
#include "XmlTableView.h"

void XmlTableView::createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, 9, x, y, w, h);
    (void)minValue;
    (void)maxValue;
}

// 属性表：Columns / Rows / SelectedRow（读写）+ RowCount（只读）
const std::vector<XmlPropEntry>& XmlTableView::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Columns", nullptr, XmlPropType::Columns, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTableView&>(b).SetColumnsProp(v); },
         nullptr},
        {L"Rows", nullptr, XmlPropType::Rows, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTableView&>(b).SetRowsProp(v); },
         nullptr},
        {L"SelectedIndex", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) { return static_cast<XmlTableView&>(b).SetSelectedRowProp(v); },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTableView&>(b).GetSelectedRowProp(); }},
        {L"RowCount", nullptr, XmlPropType::Int, nullptr,
         nullptr,
         [](const XmlMfcControlBase& b) { return static_cast<const XmlTableView&>(b).GetRowCountProp(); }},
    };
    return kTable;
}

bool XmlTableView::SetColumnsProp(const std::wstring& v)
{
    if (m_host != nullptr)
        m_host->setTableColumns(m_id, v);
    return true;
}

bool XmlTableView::SetRowsProp(const std::wstring& v)
{
    if (m_host == nullptr)
        return true;
    std::wstring cur;
    for (wchar_t ch : v)
    {
        if (ch == L'|' || ch == L'\n' || ch == L'\r')
        {
            if (!cur.empty())
                m_host->addTableRow(m_id, cur);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }
    if (!cur.empty())
        m_host->addTableRow(m_id, cur);
    return true;
}

bool XmlTableView::SetSelectedRowProp(const std::wstring& v)
{
    if (m_host != nullptr)
        m_host->setTableSelectedRow(m_id, xmlprop2::ToInt(v, -1));
    return true;
}

std::wstring XmlTableView::GetSelectedRowProp() const
{
    if (m_host == nullptr)
        return L"-1";
    return xmlprop2::FromInt(m_host->getTableSelectedRow(m_id));
}

std::wstring XmlTableView::GetRowCountProp() const
{
    if (m_host == nullptr)
        return L"0";
    return xmlprop2::FromInt(m_host->getTableRowCount(m_id));
}
