#pragma once
// XamlPageIndicator —— XAML <PageIndicator> 页码点（kind 5）。
// 属性表驱动：Count、SelectedIndex。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlPageIndicator : public XamlMfcControlBase
{
public:
    XamlPageIndicator() : XamlMfcControlBase(L"pageindicator") {}

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

    int m_count = 3;
    int m_selectedIndex = 0;
};