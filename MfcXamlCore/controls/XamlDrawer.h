#pragma once
// XamlDrawer —— XAML <XDrawer> 控件（对齐 QML Drawer）。属性表驱动：Edge、Modal、Position。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlDrawer : public XamlMfcControlBase
{
public:
    XamlDrawer() : XamlMfcControlBase(L"Drawer") {}

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

    std::wstring m_edge = L"Left";
    bool m_modal = false;
    int m_position = 0;
};
