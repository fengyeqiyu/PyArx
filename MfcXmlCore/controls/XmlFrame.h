#pragma once
// XmlFrame —— XML <XFrame> 控件（对齐 QML Frame）。属性表驱动：Padding。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlFrame : public XmlMfcControlBase
{
public:
    XmlFrame() : XmlMfcControlBase(L"Frame") {}

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

    int m_padding = 0;
};
