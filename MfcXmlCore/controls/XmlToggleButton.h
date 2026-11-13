#pragma once
// XmlToggleButton —— XML <ToggleButton> 切换按钮。属性表驱动：Text、Checked。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlToggleButton : public XmlMfcControlBase
{
public:
    XmlToggleButton() : XmlMfcControlBase(L"ToggleButton") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    std::wstring m_text;
    bool m_checked = false;
};