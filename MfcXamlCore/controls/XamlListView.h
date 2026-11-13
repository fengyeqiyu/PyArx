#pragma once
// XamlListView —— XAML <VirtualList> 虚拟化列表（kind 16）。
// 属性表驱动：Items（";"或换行分隔，一行 "标题|副标题|值"）、ItemHeight、SelectedIndex、ItemCount（只读）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlListView : public XamlMfcControlBase
{
public:
    XamlListView() : XamlMfcControlBase(L"virtuallist") {}

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
    int m_itemH = 44;
    int m_selectedIndex = -1;
};