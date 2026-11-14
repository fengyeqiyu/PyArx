#pragma once
// XmlTableDraw —— 表格类控件的唯一画法（控件库，header-only，无 MFC 依赖）。
//
// XML 语法（与引擎一致）：
//   Columns="名称:180|状态:100"       列标题 + 可选列宽（冒号后纯数字）
//   Rows="甲<TAB>1|乙<TAB>2"          行用 | 分隔，单元格用 TAB 分隔
//   Sort="名称:desc,状态:asc"         多键排序，键 = 列标题或 0 基列序号，缺省升序
//
// 设计器画布、预览器、运行时都调这一份，观感不可能再分叉。
#include <algorithm>
#include <string>
#include <vector>

#include "XmlDraw.h"

namespace xmltable
{
    inline std::vector<std::wstring> splitBy(const std::wstring& s, wchar_t sep)
    {
        std::vector<std::wstring> out;
        size_t start = 0;
        while (true)
        {
            const size_t i = s.find(sep, start);
            const std::wstring seg = (i == std::wstring::npos) ? s.substr(start) : s.substr(start, i - start);
            if (!seg.empty()) out.push_back(seg);
            if (i == std::wstring::npos) break;
            start = i + 1;
        }
        return out;
    }

    struct Style
    {
        COLORREF bg      = RGB(0xFF, 0xFF, 0xFF);
        COLORREF ink     = RGB(0x25, 0x2B, 0x33);
        COLORREF headBg  = RGB(0xF3, 0xF5, 0xF8);
        COLORREF grid    = RGB(0xDC, 0xE2, 0xEA);
        COLORREF border  = RGB(0xCF, 0xD6, 0xDF);
        int      pt      = 9;
        std::wstring family = L"Microsoft YaHei";
    };

    // 画一张表；返回 false 表示 Columns 没声明（调用方按自己的占位方式处理）
    inline bool drawTable(XmlCanvas& cv, const RECT& rc,
                          const std::wstring& colsAttr, const std::wstring& rowsAttr,
                          const std::wstring& sortAttr, const Style& st = Style())
    {
        struct Col { std::wstring title; int w; };
        std::vector<Col> cols;
        for (auto& seg : splitBy(colsAttr, L'|'))
        {
            Col c;
            c.title = seg;
            c.w = 120;
            const size_t colon = seg.rfind(L':');
            if (colon != std::wstring::npos)
            {
                const std::wstring wt = seg.substr(colon + 1);
                if (!wt.empty() && wt.find_first_not_of(L"0123456789") == std::wstring::npos)
                {
                    c.w = (std::max)(20, _wtoi(wt.c_str()));
                    c.title = seg.substr(0, colon);
                }
            }
            cols.push_back(c);
        }
        if (cols.empty()) return false;

        // 多键排序：两边都是数字时按数值比，否则按字符串比；写法顺序即优先级
        struct SortKey { int col; bool desc; };
        std::vector<SortKey> sortKeys;
        for (auto& seg : splitBy(sortAttr, L','))
        {
            std::wstring item = seg;
            while (!item.empty() && (item.front() == L' ' || item.front() == L'\t')) item.erase(item.begin());
            while (!item.empty() && (item.back() == L' ' || item.back() == L'\t')) item.pop_back();
            if (item.empty()) continue;
            bool desc = false;
            const size_t colon = item.find(L':');
            if (colon != std::wstring::npos)
            {
                std::wstring dir = item.substr(colon + 1);
                for (auto& ch : dir) ch = (wchar_t)towlower(ch);
                desc = (dir == L"desc" || dir == L"d" || dir == L"降序" || dir == L"-1");
                item = item.substr(0, colon);
            }
            int colIdx = -1;
            if (!item.empty() && item.find_first_not_of(L"0123456789") == std::wstring::npos)
            {
                colIdx = _wtoi(item.c_str());
                if (colIdx < 0 || colIdx >= (int)cols.size()) colIdx = -1;
            }
            if (colIdx < 0)
            {
                for (size_t ci = 0; ci < cols.size(); ++ci)
                    if (_wcsicmp(cols[ci].title.c_str(), item.c_str()) == 0) { colIdx = (int)ci; break; }
            }
            if (colIdx >= 0) sortKeys.push_back({ colIdx, desc });
        }

        std::vector<std::vector<std::wstring>> data;
        for (auto& rtext : splitBy(rowsAttr, L'|'))
            data.push_back(splitBy(rtext, L'\t'));

        if (!sortKeys.empty() && data.size() > 1)
        {
            auto numOf = [](const std::wstring& s, double& out) -> bool {
                if (s.empty()) return false;
                wchar_t* end = nullptr;
                out = wcstod(s.c_str(), &end);
                return end != nullptr && *end == L'\0';
            };
            auto cellOf = [](const std::vector<std::wstring>& row, int ci) -> const std::wstring& {
                static const std::wstring kEmpty;
                return (ci >= 0 && ci < (int)row.size()) ? row[ci] : kEmpty;
            };
            std::stable_sort(data.begin(), data.end(),
                [&](const std::vector<std::wstring>& A, const std::vector<std::wstring>& B) {
                    for (const auto& k : sortKeys)
                    {
                        const std::wstring& a = cellOf(A, k.col);
                        const std::wstring& b = cellOf(B, k.col);
                        double na = 0, nb = 0;
                        const bool an = numOf(a, na);
                        const bool bn = numOf(b, nb);
                        int cmp = 0;
                        if (an && bn) cmp = (na < nb) ? -1 : ((na > nb) ? 1 : 0);
                        else cmp = _wcsicmp(a.c_str(), b.c_str());
                        if (cmp != 0) return k.desc ? (cmp > 0) : (cmp < 0);
                    }
                    return false;
                });
        }

        const int padX = 4, padY = 4;
        const int left = rc.left + padX;
        const int availW = (std::max<int>)(1, (int)(rc.right - rc.left) - padX * 2);
        int total = 0;
        for (auto& c : cols) total += c.w;
        if (total > availW && total > 0)
        {
            for (auto& c : cols) c.w = (std::max)(20, c.w * availW / total);
            total = 0;
            for (auto& c : cols) total += c.w;
        }
        if (total < availW) cols.back().w += availW - total;

        const int headH = (std::max)(20, st.pt + 12);
        const int rowH = (std::max)(18, st.pt + 10);
        const int top = rc.top + padY;
        const int bottom = rc.bottom - padY;

        cv.clipPush(rc);
        cv.fillRect(RECT{ left, top, left + availW, top + headH }, st.headBg);
        int x = left;
        for (size_t ci = 0; ci < cols.size(); ++ci)
        {
            Col& c = cols[ci];
            RECT cell = { x + 6, top, x + c.w - 4, top + headH };
            cv.clipPush(cell);
            cv.text(cell, c.title, st.ink, DT_LEFT, st.pt, st.family);
            cv.clipPop();
            // 排序标记：▲升 / ▼降，多键时带优先级序号
            for (size_t ki = 0; ki < sortKeys.size(); ++ki)
            {
                if (sortKeys[ki].col != (int)ci) continue;
                std::wstring mark = sortKeys[ki].desc ? L"\u25BC" : L"\u25B2";
                if (sortKeys.size() > 1) mark += std::to_wstring(ki + 1);
                RECT mr = { x + c.w - 34, top, x + c.w - 4, top + headH };
                cv.text(mr, mark, st.ink, DT_RIGHT, 0, std::wstring());
                break;
            }
            if (x > left)
                cv.strokeLine(POINT{ x, top }, POINT{ x, bottom }, st.grid, 1);
            x += c.w;
        }
        cv.strokeLine(POINT{ left, top + headH }, POINT{ left + availW, top + headH }, st.grid, 1);

        int y = top + headH;
        for (auto& cells : data)
        {
            if (y + rowH > bottom) break;
            x = left;
            for (size_t ci = 0; ci < cols.size(); ++ci)
            {
                RECT cell = { x + 6, y, x + cols[ci].w - 4, y + rowH };
                cv.clipPush(cell);
                const std::wstring val = ci < cells.size() ? cells[ci] : std::wstring();
                if (!val.empty()) cv.text(cell, val, st.ink, DT_LEFT, st.pt, st.family);
                cv.clipPop();
                x += cols[ci].w;
            }
            y += rowH;
            cv.strokeLine(POINT{ left, y }, POINT{ left + availW, y }, st.grid, 1);
        }
        cv.clipPop();
        return true;
    }
}
