#pragma once
#include "XamlMfcControlBase.h"
#include "XamlContentParse.h"

// <ScrollBar> 可单独摆放的滚动条（不是 ScrolledWindow 内置的那条）
// 属性走 XamlTypes.h 的属性表（S1）：Minimum、Maximum、Value|Position、PageSize|Page、Orientation。
class XAML_CORE_API XamlScrollBar : public XamlMfcControlBase
{

public:
    XamlScrollBar() : XamlMfcControlBase(L"scrollbar") {}

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

    void pushRange()
    {
        if (m_host != nullptr)
            m_host->setScrollBarRange(m_id, m_vmin, m_vmax, m_pg, m_vert);
    }

    int m_vmin = 0;
    int m_vmax = 100;
    int m_pg = 10;
    bool m_vert = false;
    int m_value = 0;
    bool m_rangeSet = false;
    bool m_valueSet = false;
};
