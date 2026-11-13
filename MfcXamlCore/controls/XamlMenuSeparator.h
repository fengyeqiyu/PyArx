#pragma once
// XamlMenuSeparator —— XAML <XMenuSeparator> 控件（对齐 QML MenuSeparator）。属性表驱动：Orientation。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlMenuSeparator : public XamlMfcControlBase
{
public:
    XamlMenuSeparator() : XamlMfcControlBase(L"MenuSeparator") {}

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

    std::wstring m_orient = L"Horizontal";
};
