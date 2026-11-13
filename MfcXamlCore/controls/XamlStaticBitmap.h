#pragma once
// XamlStaticBitmap —— XAML <StaticBitmap> 静态位图。属性表驱动：Image。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlStaticBitmap : public XamlMfcControlBase
{
public:
    XamlStaticBitmap() : XamlMfcControlBase(L"staticbitmap") {}

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

    std::wstring m_image;
};