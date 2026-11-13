#pragma once
// XmlListBox —— XML <ListBox> 的标准 MFC 实现。
// 属性表驱动：Items、SelectedIndex、SelectionMode（Single|Multiple）。
// 事件（契约见 XmlEvents.h）：SelectionChanged / DoubleClick / Scroll /
// GotFocus / LostFocus。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlListBox : public XmlMfcControlBase
{
public:
    XmlListBox() : XmlMfcControlBase(L"listbox") {}

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

    void fillItems();
    // 选择模式变化要换 LBS_MULTIPLESEL，只能重建
    void rebuild();

    std::wstring m_itemsCache;
    int m_selectedIndex = -1;
    bool m_multiple = false;
};