#pragma once
// XmlCalendarCtrl —— XML <CalendarCtrl> 日历控件。
// 自有属性：Value（选中日期 "YYYY-MM-DD"，读写）；外观/层次由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlCalendarCtrl : public XmlMfcControlBase
{
public:
    XmlCalendarCtrl() : XmlMfcControlBase(L"MonthGrid") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    std::wstring m_value;
};
