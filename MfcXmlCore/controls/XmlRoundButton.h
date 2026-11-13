#pragma once
// XmlRoundButton —— XML <XRoundButton> 控件（对齐 QML RoundButton）。属性表驱动：Text、Radius。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlRoundButton : public XmlMfcControlBase
{
public:
    XmlRoundButton() : XmlMfcControlBase(L"RoundButton") {}

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
    int m_radius = 20;
};
