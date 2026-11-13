#pragma once
// XamlGroupBox —— XAML <GroupBox> 的标准 MFC 实现（分组框）。
// 属性表驱动：Header（标题）。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlGroupBox : public XamlMfcControlBase
{
public:
    XamlGroupBox() : XamlMfcControlBase(L"GroupBox") {}

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

    std::wstring m_header;
};