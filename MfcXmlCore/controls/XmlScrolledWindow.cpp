// XmlScrolledWindow.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlScrolledWindow.h"

void XmlScrolledWindow::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addScrolledWindow(id, x, y, w, h);
    applyRange();
}

const std::vector<XmlPropEntry>& XmlScrolledWindow::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"ScrollMin", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlScrolledWindow&>(b);
             c.m_minValue = xmlprop2::ToInt(v, 0);
             c.applyRange();
             return true;
         },
         [](const XmlMfcControlBase& b) { return std::to_wstring(static_cast<const XmlScrolledWindow&>(b).m_minValue); }},
        {L"ScrollMax", nullptr, XmlPropType::Int, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlScrolledWindow&>(b);
             c.m_maxValue = xmlprop2::ToInt(v, 0);
             c.applyRange();
             return true;
         },
         [](const XmlMfcControlBase& b) { return std::to_wstring(static_cast<const XmlScrolledWindow&>(b).m_maxValue); }},
    };
    return kTable;
}