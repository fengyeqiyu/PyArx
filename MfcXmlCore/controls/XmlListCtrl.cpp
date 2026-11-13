// XmlListCtrl.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlListCtrl.h"

void XmlListCtrl::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addListCtrl(id, x, y, w, h);
    if (!m_columns.empty()) applyColumns();
    if (!m_rows.empty()) applyRows();
}

void XmlListCtrl::applyColumns()
{
    if (m_host != nullptr) xmlextra::addColumns(m_host, m_id, m_columns);
}

void XmlListCtrl::applyRows()
{
    if (m_host != nullptr) xmlextra::addRows(m_host, m_id, m_rows);
}

const std::vector<XmlPropEntry>& XmlListCtrl::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Columns", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlListCtrl&>(b);
             c.m_columns = v;
             c.applyColumns();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlListCtrl&>(b).m_columns; }},
        {L"Rows", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlListCtrl&>(b);
             c.m_rows = v;
             c.applyRows();
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlListCtrl&>(b).m_rows; }},
    };
    return kTable;
}