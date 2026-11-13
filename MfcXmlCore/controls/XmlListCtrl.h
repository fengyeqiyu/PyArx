#pragma once
// XmlListCtrl —— XML <ListCtrl> 列表控件。
// 属性表驱动：Columns、Rows。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

class XML_CORE_API XmlListCtrl : public XmlMfcControlBase
{
public:
    XmlListCtrl() : XmlMfcControlBase(L"listctrl") {}

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

    void applyColumns();
    void applyRows();

    std::wstring m_columns;
    std::wstring m_rows;
};