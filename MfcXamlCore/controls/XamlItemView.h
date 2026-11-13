#pragma once
// XamlItemView —— QML ListView 的写法对齐（delegate 行模板）。
// 写法：<XItemView model="..." itemHeight="44"> 子节点即行模板（与 QML 的 delegate 子项一致）。
// 属性表：Model（; 或换行分行，行内 | 分字段）、ItemHeight、SelectedIndex、ItemCount（只读）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlItemView : public XamlMfcControlBase
{
public:
    XamlItemView() : XamlMfcControlBase(L"itemview") {}

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

    std::wstring m_model;
    int m_itemH = 44;
    int m_selectedIndex = -1;
};
