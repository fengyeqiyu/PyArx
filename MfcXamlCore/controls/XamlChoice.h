#pragma once
// XamlChoice —— XAML <Choice> 下拉选择（同 ComboBox 下拉，独立类型）。
// 属性表驱动：Items。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlChoice : public XamlMfcControlBase
{
public:
    XamlChoice() : XamlMfcControlBase(L"choice") {}

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

    void fillItems();

    std::wstring m_items;
};