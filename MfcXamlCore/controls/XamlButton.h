#pragma once
// XamlButton —— XAML <Button> 的标准 MFC 实现。
// 属性表驱动：Text。事件：Click（宿主绑定）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlButton : public XamlMfcControlBase
{
public:
    XamlButton() : XamlMfcControlBase(L"button") {}

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

    std::wstring m_text;
};