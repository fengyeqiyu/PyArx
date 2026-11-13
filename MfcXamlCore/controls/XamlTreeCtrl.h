#pragma once
// XamlTreeCtrl —— XAML <TreeCtrl> 树控件。属性表驱动：Nodes。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

class XAML_CORE_API XamlTreeCtrl : public XamlMfcControlBase
{
public:
    XamlTreeCtrl() : XamlMfcControlBase(L"TreeView") {}

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

    void applyNodes();

    std::wstring m_nodes;
};