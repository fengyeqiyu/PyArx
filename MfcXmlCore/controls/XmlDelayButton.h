#pragma once
// XmlDelayButton —— XML <XDelayButton> 控件（对齐 QML DelayButton）。属性表驱动：Text、Delay、Checked。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlDelayButton : public XmlMfcControlBase
{
public:
    XmlDelayButton() : XmlMfcControlBase(L"DelayButton") {}

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

    std::wstring m_text = L"";
    int m_delay = 3000;
    bool m_checked = false;
};
