#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

// <RangeSlider> 双滑块区间
// 属性走 XmlTypes.h 的属性表（S1）：LowValue|From、HighValue|To、Minimum|Min、Maximum|Max。
class XML_CORE_API XmlRangeSlider : public XmlMfcControlBase
{

public:
    XmlRangeSlider() : XmlMfcControlBase(L"rangeslider") {}

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

    int m_low = 0;
    int m_high = 0;
    bool m_lowSet = false;
    bool m_highSet = false;
};
