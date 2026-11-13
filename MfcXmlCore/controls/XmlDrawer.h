#pragma once
// XmlDrawer —— XML <XDrawer> 控件（对齐 QML Drawer）。属性表驱动：Edge、Modal、Position。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlDrawer : public XmlMfcControlBase
{
public:
    XmlDrawer() : XmlMfcControlBase(L"Drawer") {}

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

    std::wstring m_edge = L"Left";
    bool m_modal = false;
    int m_position = 0;
};
