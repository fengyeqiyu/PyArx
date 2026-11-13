#pragma once
// XmlMenuSeparator —— XML <XMenuSeparator> 控件（对齐 QML MenuSeparator）。属性表驱动：Orientation。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlMenuSeparator : public XmlMfcControlBase
{
public:
    XmlMenuSeparator() : XmlMfcControlBase(L"MenuSeparator") {}

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

    std::wstring m_orient = L"Horizontal";
};
