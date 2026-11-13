#pragma once
// XamlCalendarCtrl —— XAML <CalendarCtrl> 日历控件。
// 自有属性：Value（选中日期 "YYYY-MM-DD"，读写）；外观/层次由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlCalendarCtrl : public XamlMfcControlBase
{
public:
    XamlCalendarCtrl() : XamlMfcControlBase(L"CalendarCtrl") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    std::wstring m_value;
};
