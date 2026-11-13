#pragma once
// XamlFrame —— XAML <XFrame> 控件（对齐 QML Frame）。属性表驱动：Padding。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlFrame : public XamlMfcControlBase
{
public:
    XamlFrame() : XamlMfcControlBase(L"Frame") {}

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

    int m_padding = 0;
};
