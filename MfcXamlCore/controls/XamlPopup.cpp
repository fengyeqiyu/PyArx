// XamlPopup.cpp —— 属性表实现（UTF-8 with BOM）
#include "XamlPopup.h"

void XamlPopup::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 15, x, y, w, h);
}

const std::vector<XamlPropEntry>& XamlPopup::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Title", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             static_cast<XamlPopup&>(b).m_popTitle = v;
             return true;
         },
         [](const XamlMfcControlBase& b) { return static_cast<const XamlPopup&>(b).m_popTitle; }},
        {L"Content", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             static_cast<XamlPopup&>(b).m_popLines = xamlSplitItems(v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             std::wstring out;
             for (const auto& s : static_cast<const XamlPopup&>(b).m_popLines)
             {
                 if (!out.empty()) out.push_back(L'|');
                 out += s;
             }
             return out;
         }},
        {L"Lines", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             static_cast<XamlPopup&>(b).m_popLines = xamlSplitItems(v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             std::wstring out;
             for (const auto& s : static_cast<const XamlPopup&>(b).m_popLines)
             {
                 if (!out.empty()) out.push_back(L'|');
                 out += s;
             }
             return out;
         }},
    };
    return kTable;
}