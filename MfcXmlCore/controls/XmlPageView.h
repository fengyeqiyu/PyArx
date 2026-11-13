#pragma once
// XmlPageView —— XML <Page>/<StackView>/<SwipeView> 页容器（kind 17）。
// 属性表驱动：SelectedIndex、PageCount（只读）。子控件分组由 Python addPage 完成。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlPageView : public XmlMfcControlBase
{
public:
    explicit XmlPageView(const wchar_t* tag) : XmlMfcControlBase(tag) {}

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

    int m_selectedIndex = 0;
};