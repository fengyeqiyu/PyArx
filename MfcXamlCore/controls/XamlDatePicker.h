#pragma once
// XamlDatePicker —— XAML <DatePicker> 日期选择。属性表驱动：Value。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlDatePicker : public XamlMfcControlBase
{
public:
    XamlDatePicker() : XamlMfcControlBase(L"datepicker") {}

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