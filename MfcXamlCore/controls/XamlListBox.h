#pragma once
// XamlListBox —— XAML <ListBox> 的标准 MFC 实现。
// 属性表驱动：Items、SelectedIndex、SelectionMode（Single|Multiple）。
// 事件（契约见 XamlEvents.h）：SelectionChanged / DoubleClick / Scroll /
// GotFocus / LostFocus。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlListBox : public XamlMfcControlBase
{
public:
    XamlListBox() : XamlMfcControlBase(L"listbox") {}

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

    void fillItems();
    // 选择模式变化要换 LBS_MULTIPLESEL，只能重建
    void rebuild();

    std::wstring m_itemsCache;
    int m_selectedIndex = -1;
    bool m_multiple = false;
};