#pragma once
// XmlTabControl —— XML <TabControl> 选项卡容器。
// 属性表驱动：Pages（"页1|页2|…"）。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlTabControl : public XmlMfcControlBase
{
public:
    XmlTabControl() : XmlMfcControlBase(L"TabBar") {}

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

    void addPages();

    std::wstring m_pages;
};