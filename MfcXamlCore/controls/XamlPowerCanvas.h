#pragma once
// XamlPowerCanvas —— XAML <PowerCanvas> 配电规划图画布。
// 属性表驱动：Data（形如 SVG 以 '<' 开头、布局 JSON 以 '{' 开头）。只写、可读回缓存值。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlPowerCanvas : public XamlMfcControlBase
{
public:
    XamlPowerCanvas() : XamlMfcControlBase(L"powercanvas") {}

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

    void applyData();

    std::wstring m_data;
};