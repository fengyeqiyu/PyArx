#pragma once
// XmlDatePicker —— XML <DatePicker> 日期选择。属性表驱动：Value。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlDatePicker : public XmlMfcControlBase
{
public:
    XmlDatePicker() : XmlMfcControlBase(L"datepicker") {}

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