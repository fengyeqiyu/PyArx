#pragma once
// XmlProgressBar —— XML <ProgressBar>/<Progress> 的标准 MFC 实现。
// 属性走 XmlTypes.h 的属性表（S1）：Minimum|Min、Maximum|Max、Value|Position。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlProgressBar : public XmlMfcControlBase
{

public:
    XmlProgressBar() : XmlMfcControlBase(L"ProgressBar") {}

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

    int m_value = 0;
    bool m_valueSet = false;
};
