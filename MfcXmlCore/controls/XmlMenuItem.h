#pragma once
// XmlMenuItem —— XML <XMenuItem> 控件（对齐 QML MenuItem）。属性表驱动：Text、Checkable、Checked。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlMenuItem : public XmlMfcControlBase
{
public:
    XmlMenuItem() : XmlMfcControlBase(L"MenuItem") {}

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
    bool m_checkable = false;
    bool m_checked = false;
};
