#pragma once
// XmlScrolledWindow —— XML <ScrolledWindow> 滚动面板。
// 属性表驱动：ScrollMin、ScrollMax。编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlScrolledWindow : public XmlMfcControlBase
{
public:
    XmlScrolledWindow() : XmlMfcControlBase(L"scrolledwindow") {}

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

    void applyRange()
    {
        if (m_host == nullptr || m_maxValue <= m_minValue) return;
        m_host->setScrollRange(m_id, m_minValue, m_maxValue);
    }

    int m_minValue = 0;
    int m_maxValue = 0;
};