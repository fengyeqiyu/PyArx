#pragma once
// XmlSwitch —— XML <Switch> 的标准 MFC 实现（QML Switch / WPF ToggleSwitch）。
// 属性表驱动：Checked。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlSwitch : public XmlMfcControlBase
{
public:
    XmlSwitch() : XmlMfcControlBase(L"switch") {}

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

    bool m_checked = false;
};