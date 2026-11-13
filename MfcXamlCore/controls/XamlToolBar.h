#pragma once
// XamlToolBar —— XAML <ToolStrip> 工具栏（kind 11）。属性表驱动：Items、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlToolBar : public XamlMfcControlBase
{
public:
    XamlToolBar() : XamlMfcControlBase(L"ToolStrip") {}

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

    void applyItems();

    std::wstring m_items;
    int m_selectedIndex = -1;
};