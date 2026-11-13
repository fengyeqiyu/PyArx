#pragma once
// XmlTreeView —— XML <TreeTable> 层级表格（kind 10）。
// 属性表驱动：Nodes（"文字@层级|..."，末尾 '>' 表示有子节点）、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlTreeView : public XmlMfcControlBase
{
public:
    XmlTreeView() : XmlMfcControlBase(L"TreeTable") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    void applyNodes();

    std::wstring m_nodes;
    int m_selectedIndex = -1;
};