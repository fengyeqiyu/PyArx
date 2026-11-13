#pragma once
// XmlPageIndicator —— XML <PageIndicator> 页码点（kind 5）。
// 属性表驱动：Count、SelectedIndex。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlPageIndicator : public XmlMfcControlBase
{
public:
    XmlPageIndicator() : XmlMfcControlBase(L"pageindicator") {}

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

    int m_count = 3;
    int m_selectedIndex = 0;
};