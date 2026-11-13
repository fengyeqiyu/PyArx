#pragma once
// XmlScrollIndicator —— XML <XScrollIndicator> 控件（对齐 QML ScrollIndicator）。属性表驱动：Orientation、Size、Position。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlScrollIndicator : public XmlMfcControlBase
{
public:
    XmlScrollIndicator() : XmlMfcControlBase(L"ScrollIndicator") {}

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

    std::wstring m_orient = L"Vertical";
    int m_size = 0.0;
    int m_position = 0.0;
};
