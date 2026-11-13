#pragma once
// XamlListBox —— XAML <ListBox> 的标准 MFC 实现。
//
// 基础功能：条目列表（Items="a|b|c"）、当前选中项（SelectedIndex）、
// 选择模式（SelectionMode = Single 单选 / Multiple 多选）；
// 事件（契约见 XamlEvents.h）：SelectionChanged / DoubleClick / Scroll /
// GotFocus / LostFocus。
//
// 编码：本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

class XAML_CORE_API XamlListBox : public XamlMfcControlBase
{

public:
    XamlListBox() : XamlMfcControlBase(L"listbox") {}

protected:
        void createNative(IXamlMfcHost* host, int id, int , int , int x, int y, int w, int h) override;


        std::vector<XamlProperty> ownProperties() const override;


        bool setOwnProperty(const wchar_t* name, const std::wstring& value) override;


        std::wstring getOwnProperty(const wchar_t* name) const override;


private:
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

    // 选择模式变化要换 LBS_MULTIPLESEL，只能重建
    void rebuild()
    {
        if (m_host == nullptr) return;
        int x = 0, y = 0, w = 0, h = 0;
        getBounds(x, y, w, h);
        if (w <= 0 || h <= 0) return;
        m_host->addListBoxAdvanced(m_id, m_multiple, x, y, w, h);
        fillItems();
        if (m_selectedIndex >= 0)
            m_host->setCurSel(m_id, m_selectedIndex);
        pushAppearance();
    }

    std::wstring m_itemsCache;
    int m_selectedIndex = -1;
    bool m_multiple = false;
};
