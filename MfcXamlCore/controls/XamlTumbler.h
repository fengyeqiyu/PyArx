#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <Tumbler> 三行滚动选择：点上下半区或滚轮换项，循环
// 属性走 XamlTypes.h 的属性表（S1）：Count|ModelCount、CurrentIndex|Index|Value。
class XAML_CORE_API XamlTumbler : public XamlMfcControlBase
{

public:
    XamlTumbler() : XamlMfcControlBase(L"tumbler") {}

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

    int m_count = 10;
    int m_index = 0;
    bool m_indexSet = false;
};
