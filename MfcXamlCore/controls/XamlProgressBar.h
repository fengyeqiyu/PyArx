#pragma once
// XamlProgressBar —— XAML <ProgressBar>/<Progress> 的标准 MFC 实现。
// 属性走 XamlTypes.h 的属性表（S1）：Minimum|Min、Maximum|Max、Value|Position。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlProgressBar : public XamlMfcControlBase
{

public:
    XamlProgressBar() : XamlMfcControlBase(L"ProgressBar") {}

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

    int m_value = 0;
    bool m_valueSet = false;
};
