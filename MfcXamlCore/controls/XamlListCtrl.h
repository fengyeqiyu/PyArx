#pragma once
// XamlListCtrl —— XAML <ListCtrl> 列表控件。
// 属性表驱动：Columns、Rows。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

class XAML_CORE_API XamlListCtrl : public XamlMfcControlBase
{
public:
    XamlListCtrl() : XamlMfcControlBase(L"listctrl") {}

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

    void applyColumns();
    void applyRows();

    std::wstring m_columns;
    std::wstring m_rows;
};