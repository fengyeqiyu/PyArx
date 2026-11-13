// XamlCalendarCtrl.cpp —— 属性表实现（UTF-8 with BOM）
// 自有属性：Value（选中日期，格式与 DatePicker 一致："YYYY-MM-DD"）。
#include "XamlCalendarCtrl.h"

void XamlCalendarCtrl::createNative(IXamlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCalendar(id, x, y, w, h);
    if (!m_value.empty()) host->setCalendarDate(id, m_value);
}

const std::vector<XamlPropEntry>& XamlCalendarCtrl::PropTableData()
{
    static const std::vector<XamlPropEntry> kTable = {
        {L"Value", nullptr, XamlPropType::String, nullptr,
         [](XamlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XamlCalendarCtrl&>(b);
             c.m_value = v;
             if (c.m_host != nullptr) c.m_host->setCalendarDate(c.m_id, v);
             return true;
         },
         [](const XamlMfcControlBase& b) {
             const auto& c = static_cast<const XamlCalendarCtrl&>(b);
             if (c.m_host == nullptr) return c.m_value;
             const std::wstring live = c.m_host->getCalendarDate(c.m_id);
             return live.empty() ? c.m_value : live;
         }},
    };
    return kTable;
}
