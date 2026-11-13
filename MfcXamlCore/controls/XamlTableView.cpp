// XamlTableView.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）
#include "XamlTableView.h"

void XamlTableView::createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h)
{
    host->addSkinBox(id, 9, x, y, w, h);
    (void)minValue;
    (void)maxValue;
}

// 属性表：Columns / Rows / SelectedRow（读写）+ RowCount（只读）
const std::vector<XamlPropEntry>& XamlTableView::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Columns", nullptr, XamlPropType::Columns, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTableView&>(b).SetColumnsProp(v); },
         nullptr},
        {L"Rows", nullptr, XamlPropType::Rows, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTableView&>(b).SetRowsProp(v); },
         nullptr},
        {L"SelectedIndex", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) { return static_cast<XamlTableView&>(b).SetSelectedRowProp(v); },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTableView&>(b).GetSelectedRowProp(); }},
        {L"RowCount", nullptr, XamlPropType::Int, nullptr,
         nullptr,
         [](const XamlMfcControlBase& b) { return static_cast<const XamlTableView&>(b).GetRowCountProp(); }},
    };
    return kTable;
}

bool XamlTableView::SetColumnsProp(const std::wstring& v)
{
    if (m_host != nullptr)
        m_host->setTableColumns(m_id, v);
    return true;
}

bool XamlTableView::SetRowsProp(const std::wstring& v)
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

bool XamlTableView::SetSelectedRowProp(const std::wstring& v)
{
    if (m_host != nullptr)
        m_host->setTableSelectedRow(m_id, xamlprop2::ToInt(v, -1));
    return true;
}

std::wstring XamlTableView::GetSelectedRowProp() const
{
    if (m_host == nullptr)
        return L"-1";
    return xamlprop2::FromInt(m_host->getTableSelectedRow(m_id));
}

std::wstring XamlTableView::GetRowCountProp() const
{
    if (m_host == nullptr)
        return L"0";
    return xamlprop2::FromInt(m_host->getTableRowCount(m_id));
}
