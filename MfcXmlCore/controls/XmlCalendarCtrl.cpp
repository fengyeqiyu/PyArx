// XmlCalendarCtrl.cpp —— 属性表实现（UTF-8 with BOM）
// 自有属性：Value（选中日期，格式与 DatePicker 一致："YYYY-MM-DD"）。
#include "XmlCalendarCtrl.h"

void XmlCalendarCtrl::createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h)
{
    host->addCalendar(id, x, y, w, h);
    if (!m_value.empty()) host->setCalendarDate(id, m_value);
}

const std::vector<XmlPropEntry>& XmlCalendarCtrl::PropTableData()
{
    static const std::vector<XmlPropEntry> kTable = {
        {L"Value", nullptr, XmlPropType::String, nullptr,
         [](XmlMfcControlBase& b, const std::wstring& v) {
             auto& c = static_cast<XmlCalendarCtrl&>(b);
             c.m_value = v;
             if (c.m_host != nullptr) c.m_host->setCalendarDate(c.m_id, v);
             return true;
         },
         [](const XmlMfcControlBase& b) {
             const auto& c = static_cast<const XmlCalendarCtrl&>(b);
             if (c.m_host == nullptr) return c.m_value;
             const std::wstring live = c.m_host->getCalendarDate(c.m_id);
             return live.empty() ? c.m_value : live;
         }},
    };
    return kTable;
}
