#pragma once
// XamlPopup —— XAML <Popup>/<Dialog>/<Drawer> 弹层（kind 15，共用实现）。
// 属性表驱动：Title、Content、Lines（Content 与 Lines 等价，都是多行正文）。
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

class XAML_CORE_API XamlPopup : public XamlMfcControlBase
{
public:
    XamlPopup() : XamlMfcControlBase(L"popup") {}

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

    std::wstring m_popTitle;
    std::vector<std::wstring> m_popLines;
};