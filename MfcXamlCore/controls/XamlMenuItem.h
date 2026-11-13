#pragma once
// XamlMenuItem —— XAML <XMenuItem> 控件（对齐 QML MenuItem）。属性表驱动：Text、Checkable、Checked。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlMenuItem : public XamlMfcControlBase
{
public:
    XamlMenuItem() : XamlMfcControlBase(L"MenuItem") {}

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

    std::wstring m_text = L"";
    bool m_checkable = false;
    bool m_checked = false;
};
