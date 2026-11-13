#pragma once
// XmlToolBar —— XML <ToolStrip> 工具栏（kind 11）。属性表驱动：Items、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlToolBar : public XmlMfcControlBase
{
public:
    XmlToolBar() : XmlMfcControlBase(L"ToolBar") {}

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

    void applyItems();

    std::wstring m_items;
    int m_selectedIndex = -1;
};