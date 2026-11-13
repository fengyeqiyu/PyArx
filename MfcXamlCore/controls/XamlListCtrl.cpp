// XamlListCtrl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlListCtrl.h"

void XamlListCtrl::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addListCtrl(id, x, y, w, h);
    if (!m_columns.empty()) applyColumns();
    if (!m_rows.empty()) applyRows();
}

void XamlListCtrl::applyColumns()
{
    if (m_host != nullptr) xamlextra::addColumns(m_host, m_id, m_columns);
}

void XamlListCtrl::applyRows()
{
    if (m_host != nullptr) xamlextra::addRows(m_host, m_id, m_rows);
}

const std::vector<XamlPropEntry>& XamlListCtrl::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Columns", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListCtrl&>(b);
             c.m_columns = v;
             c.applyColumns();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlListCtrl&>(b).m_columns; }},
        {L"Rows", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlListCtrl&>(b);
             c.m_rows = v;
             c.applyRows();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlListCtrl&>(b).m_rows; }},
    };
    return kTable;
}