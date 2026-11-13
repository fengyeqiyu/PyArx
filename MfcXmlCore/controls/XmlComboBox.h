#pragma once
// XmlComboBox —— XML <ComboBox> 的标准 MFC 实现。
//
// 基础功能：下拉项列表（Items="a|b|c"）、当前显示文字（Text）、当前选中项
// （SelectedIndex）、下拉风格（DropDownStyle = DropDownList 只选 / DropDown 可手打）；
// 事件（契约见 XmlEvents.h）：SelectionChanged / TextChanged / DropDown /
// DoubleClick / GotFocus / LostFocus。
//
// 属性走 XmlTypes.h 的属性表（S1），不再手写三份平行定义。
//
// 编码：本文件 UTF-8 with BOM。

#include "XmlMfcControlBase.h"

class XML_CORE_API XmlComboBox : public XmlMfcControlBase
{

public:
    XmlComboBox() : XmlMfcControlBase(L"ComboBox") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int, int, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    bool SetItemsProp(const std::wstring& v);
    std::wstring GetItemsProp() const;
    bool SetTextProp(const std::wstring& v);
    std::wstring GetTextProp() const;
    bool SetSelectedIndexProp(const std::wstring& v);
    std::wstring GetSelectedIndexProp() const;
    bool SetDropDownStyleProp(const std::wstring& v);

    void fillItems()
    {
        if (m_host == nullptr) return;
        m_host->clearStrings(m_id);
        size_t start = 0;
        while (start <= m_itemsCache.size())
        {
            const size_t bar = m_itemsCache.find(L'|', start);
            const std::wstring item = (bar == std::wstring::npos)
                ? m_itemsCache.substr(start)
                : m_itemsCache.substr(start, bar - start);
            if (!item.empty())
                m_host->addString(m_id, item);
            if (bar == std::wstring::npos)
                break;
            start = bar + 1;
        }
    }

    // 下拉风格变了要换窗口样式（CBS_DROPDOWN ↔ CBS_DROPDOWNLIST），只能重建
    void rebuild()
    {
        if (m_host == nullptr) return;
        int x = 0, y = 0, w = 0, h = 0;
        getBounds(x, y, w, h);
        if (w <= 0 || h <= 0) return;
        m_host->addComboAdvanced(m_id, m_dropDownList, x, y, w, h);
        fillItems();
        if (m_selectedIndex >= 0)
            m_host->setCurSel(m_id, m_selectedIndex);
        else if (!m_text.empty())
            m_host->setText(m_id, m_text);
        pushAppearance();
    }

    std::wstring m_itemsCache;
    std::wstring m_text;
    int m_selectedIndex = -1;
    bool m_dropDownList = true;
};
