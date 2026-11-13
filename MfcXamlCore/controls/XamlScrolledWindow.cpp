// XamlScrolledWindow.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlScrolledWindow.h"

void XamlScrolledWindow::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addScrolledWindow(id, x, y, w, h);
    applyRange();
}

const std::vector<XamlPropEntry>& XamlScrolledWindow::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"ScrollMin", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlScrolledWindow&>(b);
             c.m_minValue = xamlprop2::ToInt(v, 0);
             c.applyRange();
             return true;
         },
         [](const XamlMfcControlBase& b) { return std::to_wstring(static_cast<const XamlScrolledWindow&>(b).m_minValue); }},
        {L"ScrollMax", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlScrolledWindow&>(b);
             c.m_maxValue = xamlprop2::ToInt(v, 0);
             c.applyRange();
             return true;
         },
         [](const XamlMfcControlBase& b) { return std::to_wstring(static_cast<const XamlScrolledWindow&>(b).m_maxValue); }},
    };
    return kTable;
}