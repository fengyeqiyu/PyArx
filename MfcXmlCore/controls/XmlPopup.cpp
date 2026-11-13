// XmlPopup.cpp —— 属性表实现（UTF-8 with BOM）
#include "XmlPopup.h"

void XmlPopup::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 15, x, y, w, h);
}

const std::vector<XmlPropEntry>& XmlPopup::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Title", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             static_cast<XmlPopup&>(b).m_popTitle = v;
             return true;
         },
         [](const XmlMfcControlBase& b) { return static_cast<const XmlPopup&>(b).m_popTitle; }},
        {L"Content", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             static_cast<XmlPopup&>(b).m_popLines = xmlSplitItems(v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             std::wstring out;
             for (const auto& s : static_cast<const XmlPopup&>(b).m_popLines)
             {
                 if (!out.empty()) out.push_back(L'|');
                 out += s;
             }
             return out;
         }},
        {L"Lines", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             static_cast<XmlPopup&>(b).m_popLines = xmlSplitItems(v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             std::wstring out;
             for (const auto& s : static_cast<const XmlPopup&>(b).m_popLines)
             {
                 if (!out.empty()) out.push_back(L'|');
                 out += s;
             }
             return out;
         }},
    };
    return kTable;
}