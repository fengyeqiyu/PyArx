// XamlGrid.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlGrid.h"

void XamlGrid::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addGrid(id, x, y, w, h);
    if (!m_columns.empty()) applyColumns();
    if (!m_rows.empty()) applyRows();
}

void XamlGrid::applyColumns()
{
    if (m_host != nullptr) xamlextra::addColumns(m_host, m_id, m_columns);
}

void XamlGrid::applyRows()
{
    if (m_host != nullptr) xamlextra::addRows(m_host, m_id, m_rows);
}

const std::vector<XamlPropEntry>& XamlGrid::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Columns", nullptr, XamlPropType::Columns, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlGrid&>(b);
             c.m_columns = v;
             c.applyColumns();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlGrid&>(b).m_columns; }},
        {L"Rows", nullptr, XamlPropType::Rows, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlGrid&>(b);
             c.m_rows = v;
             c.applyRows();
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlGrid&>(b).m_rows; }},
    };
    return kTable;
}