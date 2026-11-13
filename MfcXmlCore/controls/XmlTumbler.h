#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

// <Tumbler> 三行滚动选择：点上下半区或滚轮换项，循环
// 属性走 XmlTypes.h 的属性表（S1）：Count|ModelCount、CurrentIndex|Index|Value。
class XML_CORE_API XmlTumbler : public XmlMfcControlBase
{

public:
    XmlTumbler() : XmlMfcControlBase(L"tumbler") {}

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

    int m_count = 10;
    int m_index = 0;
    bool m_indexSet = false;
};
