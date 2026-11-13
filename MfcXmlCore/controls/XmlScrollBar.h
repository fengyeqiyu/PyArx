#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

// <ScrollBar> 可单独摆放的滚动条（不是 ScrolledWindow 内置的那条）
// 属性走 XmlTypes.h 的属性表（S1）：Minimum、Maximum、Value|Position、PageSize|Page、Orientation。
class XML_CORE_API XmlScrollBar : public XmlMfcControlBase
{

public:
    XmlScrollBar() : XmlMfcControlBase(L"scrollbar") {}

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
