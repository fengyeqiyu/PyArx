#pragma once
// XmlTreeCtrl —— XML <TreeCtrl> 树控件。属性表驱动：Nodes。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

class XML_CORE_API XmlTreeCtrl : public XmlMfcControlBase
{
public:
    XmlTreeCtrl() : XmlMfcControlBase(L"TreeView") {}

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

    void applyNodes();

    std::wstring m_nodes;
};