#pragma once
// XmlToolTip —— XML <ToolTip> 静态提示（kind 12）。属性表驱动：Text。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlToolTip : public XmlMfcControlBase
{
public:
    XmlToolTip() : XmlMfcControlBase(L"tooltip") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    std::wstring m_text;
};