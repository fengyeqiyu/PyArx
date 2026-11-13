#pragma once
// XmlPathView —— XML <PathView> 路径委托（kind 19）。
// 属性表驱动：Items（";"/换行分隔）、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlPathView : public XmlMfcControlBase
{
public:
    XmlPathView() : XmlMfcControlBase(L"pathview") {}

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

    void fillItems();

    std::wstring m_items;
    int m_selectedIndex = 0;
};