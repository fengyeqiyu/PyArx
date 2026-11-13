#pragma once
// XamlSwitch —— XAML <Switch> 的标准 MFC 实现（QML Switch / WPF ToggleSwitch）。
// 属性表驱动：Checked。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlSwitch : public XamlMfcControlBase
{
public:
    XamlSwitch() : XamlMfcControlBase(L"switch") {}

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

    bool m_checked = false;
};