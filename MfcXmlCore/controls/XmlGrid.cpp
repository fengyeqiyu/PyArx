// XmlGrid.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlGrid.h"

void XmlGrid::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addGrid(id, x, y, w, h);
    if (!m_columns.empty()) applyColumns();
    if (!m_rows.empty()) applyRows();
}

void XmlGrid::applyColumns()
{
    if (m_host != nullptr) xmlextra::addColumns(m_host, m_id, m_columns);
}

void XmlGrid::applyRows()
{
    if (m_host != nullptr) xmlextra::addRows(m_host, m_id, m_rows);
}

const std::vector<XmlPropEntry>& XmlGrid::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Columns", nullptr, XmlPropType::Columns, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGrid&>(b);
             c.m_columns = v;
             c.applyColumns();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlGrid&>(b).m_columns; }},
        {L"Rows", nullptr, XmlPropType::Rows, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlGrid&>(b);
             c.m_rows = v;
             c.applyRows();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlGrid&>(b).m_rows; }},
    };
    return kTable;
}