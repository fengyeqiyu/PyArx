#pragma once
// XamlTabControl —— XAML <TabControl> 选项卡容器。
// 属性表驱动：Pages（"页1|页2|…"）。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlTabControl : public XamlMfcControlBase
{
public:
    XamlTabControl() : XamlMfcControlBase(L"TabBar") {}

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

    void addPages();

    std::wstring m_pages;
};