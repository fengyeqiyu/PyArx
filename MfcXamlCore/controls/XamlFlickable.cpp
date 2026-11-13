// XamlFlickable.cpp —— 属性表实现（UTF-8 with BOM）
// 自有属性：OffsetX / OffsetY（读写，落到宿主的视口偏移），其余由基类统一处理。
#include "XamlFlickable.h"

void XamlFlickable::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addSkinBox(id, 21, x, y, w, h);
}

const std::vector<XamlPropEntry>& XamlFlickable::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"OffsetX", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlFlickable&>(b);
             if (c.m_host == nullptr) return false;
             c.m_host->setFlickOffset(c.m_id, xamlprop2::ToInt(v, 0),
                                      c.m_host->getFlickOffsetY(c.m_id));
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlFlickable&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getFlickOffsetX(c.m_id) : 0);
         }},
        {L"OffsetY", nullptr, XamlPropType::Int, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlFlickable&>(b);
             if (c.m_host == nullptr) return false;
             c.m_host->setFlickOffset(c.m_id, c.m_host->getFlickOffsetX(c.m_id),
                                      xamlprop2::ToInt(v, 0));
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlFlickable&>(b);
             return std::to_wstring(c.m_host != nullptr ? c.m_host->getFlickOffsetY(c.m_id) : 0);
         }},
    };
    return kTable;
}
