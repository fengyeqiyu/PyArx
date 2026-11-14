#pragma once
// XmlTableDraw —— 表格类控件的唯一画法（控件库，header-only，无 MFC 窗口依赖）。
//
// XML 语法（与引擎一致）：
//   Columns="名称:180|状态:100"       列标题 + 可选列宽（冒号后纯数字）
//   Rows="甲<TAB>1|乙<TAB>2"          行用 | 分隔，单元格用 TAB 分隔
//   Sort="名称:desc,状态:asc"         多键排序，键 = 列标题或 0 基列序号，缺省升序
//
// 拆成三段：buildTable（算几何，含排序）→ hitRow / rowRect（命中与选中）→ drawTable（上色）。
// 设计器、预览器、运行时都调这一份，观感和命中不可能再分叉。
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

    struct Col
    {
        std::wstring title;
        int w = 120;
    };

    // 一次布局的全部几何：命中、绘制、行高亮都从它取，避免两处各算一遍
    struct Table
    {
        std::vector<Col> cols;
        std::vector<std::vector<std::wstring>> rows;
        std::vector<int> sortCol;
        std::vector<bool> sortDesc;
        int headH = 22;
        int rowH = 20;
        int left = 0, top = 0, availW = 0, bottom = 0;
        bool empty() const { return cols.empty(); }
        int visibleRows() const
        {
            if (rowH <= 0) return 0;
            const int h = bottom - (top + headH);
            return h <= 0 ? 0 : h / rowH;
        }
    };

    struct Style
    {
        COLORREF bg      = RGB(0xFF, 0xFF, 0xFF);
        COLORREF ink     = RGB(0x25, 0x2B, 0x33);
        COLORREF headBg  = RGB(0xF3, 0xF5, 0xF8);
        COLORREF grid    = RGB(0xDC, 0xE2, 0xEA);
        COLORREF border  = RGB(0xCF, 0xD6, 0xDF);
        COLORREF selBg   = RGB(0xE8, 0xF0, 0xFE);   // 选中行底色
        COLORREF selInk  = RGB(0x14, 0x3C, 0x8B);   // 选中行文字
        int      pt      = 9;
        std::wstring family = L"Microsoft YaHei";
    };

    // 算几何（列宽压缩、多键排序都在这里做完）。列没声明时返回 false。
    inline bool buildTable(const RECT& rc, const std::wstring& colsAttr, const std::wstring& rowsAttr,
                           const std::wstring& sortAttr, int pt, Table& t)
    {
        t.cols.clear();
        t.rows.clear();
        t.sortCol.clear();
        t.sortDesc.clear();

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
            t.cols.push_back(c);
        }
        if (t.cols.empty()) return false;

        // 多键排序：两边都是数字时按数值比，否则按字符串比；书写顺序即优先级
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
                if (colIdx < 0 || colIdx >= (int)t.cols.size()) colIdx = -1;
            }
            if (colIdx < 0)
            {
                for (size_t ci = 0; ci < t.cols.size(); ++ci)
                    if (_wcsicmp(t.cols[ci].title.c_str(), item.c_str()) == 0) { colIdx = (int)ci; break; }
            }
            if (colIdx >= 0) { t.sortCol.push_back(colIdx); t.sortDesc.push_back(desc); }
        }

        for (auto& rtext : splitBy(rowsAttr, L'|'))
            t.rows.push_back(splitBy(rtext, L'\t'));

        if (!t.sortCol.empty() && t.rows.size() > 1)
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
            std::stable_sort(t.rows.begin(), t.rows.end(),
                [&](const std::vector<std::wstring>& A, const std::vector<std::wstring>& B) {
                    for (size_t k = 0; k < t.sortCol.size(); ++k)
                    {
                        const std::wstring& a = cellOf(A, t.sortCol[k]);
                        const std::wstring& b = cellOf(B, t.sortCol[k]);
                        double na = 0, nb = 0;
                        const bool an = numOf(a, na);
                        const bool bn = numOf(b, nb);
                        int cmp = 0;
                        if (an && bn) cmp = (na < nb) ? -1 : ((na > nb) ? 1 : 0);
                        else cmp = _wcsicmp(a.c_str(), b.c_str());
                        if (cmp != 0) return t.sortDesc[k] ? (cmp > 0) : (cmp < 0);
                    }
                    return false;
                });
        }

        const int padX = 4, padY = 4;
        t.left = (int)rc.left + padX;
        t.availW = (std::max)(1, (int)(rc.right - rc.left) - padX * 2);
        int total = 0;
        for (auto& c : t.cols) total += c.w;
        if (total > t.availW && total > 0)
        {
            for (auto& c : t.cols) c.w = (std::max)(20, c.w * t.availW / total);
            total = 0;
            for (auto& c : t.cols) total += c.w;
        }
        if (total < t.availW) t.cols.back().w += t.availW - total;

        t.headH = (std::max)(20, pt + 12);
        t.rowH = (std::max)(18, pt + 10);
        t.top = (int)rc.top + padY;
        t.bottom = (int)rc.bottom - padY;
        return true;
    }

    // 光标落在第几行（表头以上或空白处返回 -1）
    inline int hitRow(const Table& t, POINT p)
    {
        if (t.empty() || t.rowH <= 0) return -1;
        if (p.x < t.left || p.x > t.left + t.availW) return -1;
        if (p.y < t.top + t.headH || p.y >= t.bottom) return -1;
        const int idx = (p.y - (t.top + t.headH)) / t.rowH;
        if (idx < 0 || idx >= (int)t.rows.size() || idx >= t.visibleRows()) return -1;
        return idx;
    }

    inline RECT rowRect(const Table& t, int row)
    {
        RECT r = { t.left, t.top + t.headH + row * t.rowH, t.left + t.availW, 0 };
        r.bottom = r.top + t.rowH;
        return r;
    }

    inline void drawTable(XmlCanvas& cv, const RECT& rc, const Table& t, const Style& st, int selRow = -1)
    {
        if (t.empty()) return;
        cv.clipPush(rc);
        cv.fillRect(RECT{ t.left, t.top, t.left + t.availW, t.top + t.headH }, st.headBg);

        int x = t.left;
        for (size_t ci = 0; ci < t.cols.size(); ++ci)
        {
            const Col& c = t.cols[ci];
            RECT cell = { x + 6, t.top, x + c.w - 4, t.top + t.headH };
            cv.clipPush(cell);
            cv.text(cell, c.title, st.ink, DT_LEFT, st.pt, st.family);
            cv.clipPop();
            // 排序标记：▲升 / ▼降，多键时带优先级序号
            for (size_t ki = 0; ki < t.sortCol.size(); ++ki)
            {
                if (t.sortCol[ki] != (int)ci) continue;
                std::wstring mark = t.sortDesc[ki] ? L"\u25BC" : L"\u25B2";
                if (t.sortCol.size() > 1) mark += std::to_wstring(ki + 1);
                RECT mr = { x + c.w - 34, t.top, x + c.w - 4, t.top + t.headH };
                cv.text(mr, mark, st.ink, DT_RIGHT, 0, std::wstring());
                break;
            }
            if (x > t.left) cv.strokeLine(POINT{ x, t.top }, POINT{ x, t.bottom }, st.grid, 1);
            x += c.w;
        }
        cv.strokeLine(POINT{ t.left, t.top + t.headH }, POINT{ t.left + t.availW, t.top + t.headH }, st.grid, 1);

        int y = t.top + t.headH;
        for (size_t ri = 0; ri < t.rows.size(); ++ri)
        {
            if (y + t.rowH > t.bottom) break;
            const bool sel = (selRow == (int)ri);
            if (sel)
                cv.fillRect(RECT{ t.left, y, t.left + t.availW, y + t.rowH }, st.selBg);
            x = t.left;
            for (size_t ci = 0; ci < t.cols.size(); ++ci)
            {
                RECT cell = { x + 6, y, x + t.cols[ci].w - 4, y + t.rowH };
                cv.clipPush(cell);
                const std::wstring val = ci < t.rows[ri].size() ? t.rows[ri][ci] : std::wstring();
                if (!val.empty()) cv.text(cell, val, sel ? st.selInk : st.ink, DT_LEFT, st.pt, st.family);
                cv.clipPop();
                x += t.cols[ci].w;
            }
            y += t.rowH;
            cv.strokeLine(POINT{ t.left, y }, POINT{ t.left + t.availW, y }, st.grid, 1);
        }
        cv.clipPop();
    }

    // 便捷版：算 + 画一次完成（不给选中态时的老调用点用这个）
    inline bool drawTable(XmlCanvas& cv, const RECT& rc,
                          const std::wstring& colsAttr, const std::wstring& rowsAttr,
                          const std::wstring& sortAttr, const Style& st = Style(), int selRow = -1)
    {
        Table t;
        if (!buildTable(rc, colsAttr, rowsAttr, sortAttr, st.pt, t)) return false;
        drawTable(cv, rc, t, st, selRow);
        return true;
    }
}
