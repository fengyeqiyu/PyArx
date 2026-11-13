#pragma once
// XmlItemView —— QML ListView 的写法对齐（delegate 行模板）。
// 写法：<XItemView model="..." itemHeight="44"> 子节点即行模板（与 QML 的 delegate 子项一致）。
// 属性表：Model（; 或换行分行，行内 | 分字段）、ItemHeight、SelectedIndex、ItemCount（只读）。
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlItemView : public XmlMfcControlBase
{
public:
    XmlItemView() : XmlMfcControlBase(L"itemview") {}

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

    std::wstring m_model;
    int m_itemH = 44;
    int m_selectedIndex = -1;
};
