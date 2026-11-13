#pragma once
// XmlGroupBox —— XML <GroupBox> 的标准 MFC 实现（分组框）。
// 属性表驱动：Header（标题）。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlGroupBox : public XmlMfcControlBase
{
public:
    XmlGroupBox() : XmlMfcControlBase(L"GroupBox") {}

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

    std::wstring m_header;
};