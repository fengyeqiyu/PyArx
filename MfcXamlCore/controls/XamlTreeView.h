#pragma once
// XamlTreeView —— XAML <TreeTable> 层级表格（kind 10）。
// 属性表驱动：Nodes（"文字@层级|..."，末尾 '>' 表示有子节点）、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlTreeView : public XamlMfcControlBase
{
public:
    XamlTreeView() : XamlMfcControlBase(L"TreeTable") {}

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

    void applyNodes();

    std::wstring m_nodes;
    int m_selectedIndex = -1;
};