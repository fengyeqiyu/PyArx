#pragma once
// XmlDialog —— XML <XDialog> 控件（对齐 QML Dialog）。属性表驱动：Title、Modal、StandardButtons。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlDialog : public XmlMfcControlBase
{
public:
    XmlDialog() : XmlMfcControlBase(L"Dialog") {}

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

    std::wstring m_title = L"";
    bool m_modal = false;
    std::wstring m_buttons = L"";
};
