#pragma once
// XamlGridView —— XAML <GridView> 网格委托（kind 18）。
// 属性表驱动：Items（";"/换行分隔）、SelectedIndex、CellWidth、CellHeight、ItemCount（只读）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlGridView : public XamlMfcControlBase
{
public:
    XamlGridView() : XamlMfcControlBase(L"gridview") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    void fillItems();

    std::wstring m_items;
    int m_selectedIndex = 0;
    int m_cellW = 0;
    int m_cellH = 0;
};