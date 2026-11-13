#pragma once
// XamlDialog —— XAML <XDialog> 控件（对齐 QML Dialog）。属性表驱动：Title、Modal、StandardButtons。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlDialog : public XamlMfcControlBase
{
public:
    XamlDialog() : XamlMfcControlBase(L"Dialog") {}

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

    std::wstring m_title = L"";
    bool m_modal = false;
    std::wstring m_buttons = L"";
};
