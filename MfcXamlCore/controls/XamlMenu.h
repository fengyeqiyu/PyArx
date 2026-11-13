#pragma once
// XamlMenu —— XAML <Menu> 菜单（kind 13）。属性表驱动：Items、SelectedIndex（只读）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

class XAML_CORE_API XamlMenu : public XamlMfcControlBase
{
public:
    XamlMenu() : XamlMfcControlBase(L"menu") {}

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

    void applyItems();

    std::wstring m_items;
};