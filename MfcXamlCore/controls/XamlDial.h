#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Dial> 圆弧旋钮：点/拖按角度取值，滚轮步进
// 属性走 XamlTypes.h 的属性表（S1）：Minimum|From、Maximum|To、Value|Position。
class XAML_CORE_API XamlDial : public XamlMfcControlBase
{

public:
    XamlDial() : XamlMfcControlBase(L"dial") {}

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

    int m_vmin = 0;
    int m_vmax = 100;
    int m_value = 0;
    bool m_rangeSet = false;   // 属性表已经显式给过范围，createNative 不再用构造参数覆盖
    bool m_valueSet = false;
};
