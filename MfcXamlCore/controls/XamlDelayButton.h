#pragma once
// XamlDelayButton —— XAML <XDelayButton> 控件（对齐 QML DelayButton）。属性表驱动：Text、Delay、Checked。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlDelayButton : public XamlMfcControlBase
{
public:
    XamlDelayButton() : XamlMfcControlBase(L"DelayButton") {}

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
    int m_delay = 3000;
    bool m_checked = false;
};
