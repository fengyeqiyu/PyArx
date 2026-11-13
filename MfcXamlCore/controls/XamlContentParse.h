#pragma once
// XamlContentParse —— 内容类属性（Pages/Columns/Rows/Nodes）的公共解析。
// 声明了属性却不实现，XAML 里的内容会被静默丢弃，控件就只剩空壳，
// 所以这几个解析函数是"控件能用"和"只有样子"的分界线。
// 本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

namespace xamlextra
{
    inline std::vector<std::wstring> splitBar(const std::wstring& value)
    {
        std::vector<std::wstring> out;
        size_t start = 0;
        while (start <= value.size())
        {
            const size_t bar = value.find(L'|', start);
            const std::wstring seg = (bar == std::wstring::npos)
                ? value.substr(start)
                : value.substr(start, bar - start);
            if (!seg.empty())
                out.push_back(seg);
            if (bar == std::wstring::npos)
                break;
            start = bar + 1;
        }
        return out;
    }

    // Columns="名称:180|状态:100"，冒号后是列宽（省略按 120）
    inline void addColumns(IXamlMfcHost* host, int id, const std::wstring& value)
    {
        for (const auto& seg : splitBar(value))
        {
            int width = 120;
            std::wstring title = seg;
            const size_t colon = seg.rfind(L':');
            if (colon != std::wstring::npos)
            {
                const std::wstring wtext = seg.substr(colon + 1);
                if (!wtext.empty() && wtext.find_first_not_of(L"0123456789") == std::wstring::npos)
                {
                    width = _wtoi(wtext.c_str());
                    title = seg.substr(0, colon);
                }
            }
            host->addColumn(id, title, width);
        }
    }

    // Rows="甲\t1|乙\t2"：行用 | 分隔，行内单元格用 \t 分隔
    inline void addRows(IXamlMfcHost* host, int id, const std::wstring& value)
    {
        for (const auto& row : splitBar(value))
            host->addRow(id, row);
    }

    // Nodes="主干>叶子1|主干>叶子2"：一个 > 表示一层父子关系
    inline void addNodes(IXamlMfcHost* host, int id, const std::wstring& value)
    {
        std::vector<std::wstring> added;          // 已插入节点，下标即 parentIndex
        for (const auto& seg : splitBar(value))
        {
            int parent = -1;
            size_t start = 0;
            while (start <= seg.size())
            {
                const size_t gt = seg.find(L'>', start);
                const std::wstring part = (gt == std::wstring::npos)
                    ? seg.substr(start) : seg.substr(start, gt - start);
                if (!part.empty())
                {
                    host->addTreeNode(id, part, parent);
                    added.push_back(part);
                    parent = static_cast<int>(added.size()) - 1;
                }
                if (gt == std::wstring::npos)
                    break;
                start = gt + 1;
            }
        }
    }
}

// ---- 类外公共小工具（原 XamlExtraControls.h 里的自由函数） ----
// 属性里的整数字面量：解析失败就走默认值（不抛异常）
inline int xamlExtraNum(const std::wstring& s, int def)
{
    try { return std::stoi(s); } catch (...) { return def; }
}

inline std::vector<std::wstring> xamlSplitItems(const std::wstring& value)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t ch : value)
    {
        if (ch == L'|' || ch == L'\n' || ch == L'\r')
        {
            if (!cur.empty())
                out.push_back(cur);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}
