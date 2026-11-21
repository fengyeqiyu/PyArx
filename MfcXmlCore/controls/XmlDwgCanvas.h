#pragma once
// XmlDwgCanvas —— XML <XDwgCanvas> 绘图画布（DWG 为主，兼容已算坐标的布局输入）。
// 属性表驱动：Data（形如 SVG 以 '<' 开头、布局 JSON 以 '{' 开头）。只写、可读回缓存值。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlDwgCanvas : public XmlMfcControlBase
{
public:
    XmlDwgCanvas() : XmlMfcControlBase(L"dwgcanvas") {}

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

    void applyData();

    std::wstring m_data;
};