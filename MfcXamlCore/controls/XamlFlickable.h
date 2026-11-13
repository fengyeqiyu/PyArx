#pragma once
// XamlFlickable —— XAML <flickable> 可滑动视图（kind 21）。
// 自有属性：OffsetX / OffsetY（视口偏移，读写）；外观/层次由基类统一处理。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlFlickable : public XamlMfcControlBase
{
public:
    XamlFlickable() : XamlMfcControlBase(L"flickable") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();
};