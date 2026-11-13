#pragma once
// XmlPopup —— XML <Popup>/<Dialog>/<Drawer> 弹层（kind 15，共用实现）。
// 属性表驱动：Title、Content、Lines（Content 与 Lines 等价，都是多行正文）。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

class XML_CORE_API XmlPopup : public XmlMfcControlBase
{
public:
    XmlPopup() : XmlMfcControlBase(L"popup") {}

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

    std::wstring m_popTitle;
    std::vector<std::wstring> m_popLines;
};