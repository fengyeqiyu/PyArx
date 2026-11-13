#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <RangeSlider> 双滑块区间
// 属性走 XamlTypes.h 的属性表（S1）：LowValue|From、HighValue|To、Minimum|Min、Maximum|Max。
class XAML_CORE_API XamlRangeSlider : public XamlMfcControlBase
{

public:
    XamlRangeSlider() : XamlMfcControlBase(L"rangeslider") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    int m_low = 0;
    int m_high = 0;
    bool m_lowSet = false;
    bool m_highSet = false;
};
