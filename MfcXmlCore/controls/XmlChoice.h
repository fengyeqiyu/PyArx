#pragma once
// XmlChoice —— XML <Choice> 下拉选择（同 ComboBox 下拉，独立类型）。
// 属性表驱动：Items。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlChoice : public XmlMfcControlBase
{
public:
    XmlChoice() : XmlMfcControlBase(L"choice") {}

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

    std::wstring m_items;
};