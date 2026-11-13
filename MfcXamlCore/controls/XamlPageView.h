#pragma once
// XamlPageView —— XAML <Page>/<StackView>/<SwipeView> 页容器（kind 17）。
// 属性表驱动：SelectedIndex、PageCount（只读）。子控件分组由 Python addPage 完成。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlPageView : public XamlMfcControlBase
{
public:
    explicit XamlPageView(const wchar_t* tag) : XamlMfcControlBase(tag) {}

protected:
    void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    int m_selectedIndex = 0;
};