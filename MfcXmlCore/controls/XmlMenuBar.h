#pragma once
// XmlMenuBar —— XML <MenuStrip> 菜单条（kind 14）。属性表驱动：Items、SelectedIndex。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

class XML_CORE_API XmlMenuBar : public XmlMfcControlBase
{
public:
    XmlMenuBar() : XmlMfcControlBase(L"MenuBar") {}

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