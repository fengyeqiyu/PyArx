#pragma once
// XmlFlickable —— XML <flickable> 可滑动视图（kind 21）。
// 自有属性：OffsetX / OffsetY（视口偏移，读写）；外观/层次由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlFlickable : public XmlMfcControlBase
{
public:
    XmlFlickable() : XmlMfcControlBase(L"flickable") {}

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
};