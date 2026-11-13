#pragma once
// XamlScrolledWindow —— XAML <ScrolledWindow> 滚动面板。
// 属性表驱动：ScrollMin、ScrollMax。编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlScrolledWindow : public XamlMfcControlBase
{
public:
    XamlScrolledWindow() : XamlMfcControlBase(L"scrolledwindow") {}

protected:
    void createNative(IXamlMfcHost* host, int id, int min, int max, int x, int y, int w, int h) override;

    const XamlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XamlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XamlPropEntry>& PropTableData();

    void applyRange()
    {
        if (m_host == nullptr || m_maxValue <= m_minValue) return;
        m_host->setScrollRange(m_id, m_minValue, m_maxValue);
    }

    int m_minValue = 0;
    int m_maxValue = 0;
};