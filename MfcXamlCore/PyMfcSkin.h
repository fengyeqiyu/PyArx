#pragma once
// PyMfcSkin.h — 原生控件的"自绘皮肤"。
//
// 分工（这是全控件自绘的前提）：
//   * 状态与交互仍由系统类维护（焦点、键盘、输入、滚动、命中测试、选中集合），
//     PyMfcSkinProc 只把控件的 WM_PAINT 掏空，不让它自己画像素；
//   * 外观全部由这里的函数用 XamlCanvas（D2D）画出来，配色来自同一份主题。
// 这样既能"所有控件一个样子"，又不用重写每个控件的交互逻辑。
//
// 只依赖 XamlCanvas 抽象接口，和 PyMfcXaml 里那套绘制器同源。

#include <windows.h>
#include <commctrl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "XamlDraw.h"
#include "XamlControls.h"

namespace pymfcskin
{

// 一份配色喂到所有控件：主题层改一次，全界面跟着变
struct Palette
{
    COLORREF back   = RGB(49, 56, 66);    // 控件底
    COLORREF fore   = RGB(235, 235, 235); // 文字
    COLORREF border = RGB(95, 105, 118);  // 边框
    COLORREF accent = RGB(0, 120, 212);   // 进度、选中
    COLORREF muted  = RGB(62, 70, 80);    // 轨道、空槽、次要底
    int ptSize      = 9;
    std::wstring face = L"Segoe UI";
};

// 列表 / 表格的一行
struct Row
{
    std::wstring text;
    bool selected = false;
};

// 表格的一行（按列切格，ListView / GridCtrl 用）
struct GridRow
{
    std::vector<std::wstring> cells;
    bool selected = false;
};

// ---------------------------------------------------------------- 进度条
inline void drawProgressBar(XamlCanvas& cv, const RECT& rc, int pos, int minV, int maxV,
                            const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);          // 整块铺底：不留系统上次画的像素
    cv.fillRoundRect(rc, pal.muted, pal.border, 4);

    const int span = maxV > minV ? maxV - minV : 0;
    const int innerW = (rc.right - rc.left) - 4;
    int fillW = 0;
    if (span > 0)
        fillW = static_cast<int>((static_cast<long long>(innerW) * (pos - minV)) / span);
    if (fillW < 0)
        fillW = 0;
    if (fillW > innerW)
        fillW = innerW;
    if (fillW > 0)
    {
        RECT fr = { rc.left + 2, rc.top + 2, rc.left + 2 + fillW, rc.bottom - 2 };
        cv.fillRoundRect(fr, pal.accent, pal.accent, 3);
    }
}

// ---------------------------------------------------------------- 滑块
inline void drawSlider(XamlCanvas& cv, const RECT& rc, int pos, int minV, int maxV,
                       bool vertical, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);          // 整块铺底：拖动时不留系统刻度残影
    const int span = maxV > minV ? maxV - minV : 0;
    const int knob = 14;

    if (!vertical)
    {
        const int cy = (rc.top + rc.bottom) / 2;
        RECT track = { rc.left + knob / 2, cy - 2, rc.right - knob / 2, cy + 2 };
        if (track.right > track.left)
            cv.fillRoundRect(track, pal.muted, pal.border, 2);
        int kx = rc.left + knob / 2;
        if (span > 0)
        {
            const int usable = (rc.right - knob) - rc.left;
            kx += static_cast<int>((static_cast<long long>(usable) * (pos - minV)) / span);
        }
        RECT kr = { kx - knob / 2, cy - knob / 2, kx + knob / 2, cy + knob / 2 };
        cv.fillRoundRect(kr, pal.accent, pal.border, 3);
    }
    else
    {
        const int cx = (rc.left + rc.right) / 2;
        RECT track = { cx - 2, rc.top + knob / 2, cx + 2, rc.bottom - knob / 2 };
        if (track.bottom > track.top)
            cv.fillRoundRect(track, pal.muted, pal.border, 2);
        int ky = rc.bottom - knob / 2;
        if (span > 0)
        {
            const int usable = (rc.bottom - knob) - rc.top;
            ky -= static_cast<int>((static_cast<long long>(usable) * (pos - minV)) / span);
        }
        RECT kr = { cx - knob / 2, ky - knob / 2, cx + knob / 2, ky + knob / 2 };
        cv.fillRoundRect(kr, pal.accent, pal.border, 3);
    }
}

// ---------------------------------------------------------------- 列表框 / 列表
inline void drawList(XamlCanvas& cv, const RECT& rc, const std::vector<Row>& rows,
                     const Palette& pal, bool hasFocus, int rowH = 22)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.fillRoundRect(rc, pal.back, pal.border, 2);

    cv.clipPush(rc);
    int y = static_cast<int>(rc.top) + 1;
    for (const Row& r : rows)
    {
        if (y + rowH > static_cast<int>(rc.bottom))
            break;
        RECT rowRc = { rc.left + 1, y, rc.right - 1, y + rowH };
        if (r.selected)
            cv.fillRect(rowRc, pal.accent);
        RECT tr = { rowRc.left + 6, rowRc.top, rowRc.right - 4, rowRc.bottom };
        cv.text(tr, r.text, r.selected ? RGB(255, 255, 255) : pal.fore,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        y += rowH;
    }
    cv.clipPop();

    if (hasFocus)
        cv.strokeRect(rc, pal.accent, 1);
}

// ---------------------------------------------------------------- 组合框（收起状态）
inline void drawCombo(XamlCanvas& cv, const RECT& rc, const std::wstring& text,
                      const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.fillRoundRect(rc, pal.back, pal.border, 3);

    RECT tr = { rc.left + 8, rc.top, rc.right - 24, rc.bottom };
    if (!text.empty())
        cv.text(tr, text, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
                pal.ptSize, pal.face);

    // 右侧下拉箭头：两条斜线，避免依赖字体里的三角符号
    const int ax = rc.right - 14;
    const int ay = (rc.top + rc.bottom) / 2;
    cv.strokeLine(POINT{ ax - 4, ay - 2 }, POINT{ ax, ay + 2 }, pal.fore, 1);
    cv.strokeLine(POINT{ ax, ay + 2 }, POINT{ ax + 4, ay - 2 }, pal.fore, 1);
}

// ---------------------------------------------------------------- 选项卡（页签条 + 内容区）
inline void drawTab(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& titles,
                    int curSel, const Palette& pal, int tabH = 28)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;

    cv.fillRect(rc, pal.back);   // 整块铺底：页签条右侧空白不留系统白

    // 内容区：页签下面整块按主题铺底色（这正是系统会刷成白板的地方）
    RECT body = { rc.left, rc.top + tabH, rc.right, rc.bottom };
    if (body.bottom > body.top)
        cv.fillRoundRect(body, pal.back, pal.border, 3);

    // 页签条
    int x = rc.left;
    for (size_t i = 0; i < titles.size(); ++i)
    {
        const int tw = 72;
        RECT tr = { x, rc.top, x + tw, rc.top + tabH };
        const bool cur = static_cast<int>(i) == curSel;
        cv.fillRoundRect(tr, cur ? pal.muted : pal.back, pal.border, 3);
        cv.text(tr, titles[i], cur ? pal.fore : RGB(180, 188, 200),
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        x += tw + 2;
        if (x >= rc.right)
            break;
    }
}

// ---------------------------------------------------------------- 表格（ListView / GridCtrl）
// header：每列标题；widths：每列宽度（像素）。headerH = 0 表示不画表头。
inline void drawGrid(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& header,
                     const std::vector<int>& widths, const std::vector<GridRow>& rows,
                     const Palette& pal, bool hasFocus, int headerH = 22, int rowH = 22)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    cv.clipPush(rc);
    const int contentTop = static_cast<int>(rc.top) + headerH;
    int y = contentTop;
    for (const GridRow& r : rows)
    {
        if (y + rowH > static_cast<int>(rc.bottom))
            break;
        RECT rowRc = { rc.left, y, rc.right, y + rowH };
        if (r.selected)
            cv.fillRect(rowRc, pal.accent);

        int x = static_cast<int>(rc.left) + 4;
        for (size_t c = 0; c < r.cells.size(); ++c)
        {
            const int cw = (c < widths.size() && widths[c] > 0)
                               ? widths[c]
                               : (static_cast<int>(rc.right) - x) / 4;
            if (c > 0)
                cv.fillRect(RECT{ x - 3, y + 3, x - 2, y + rowH - 3 }, RGB(70, 78, 90));
            RECT tr = { x, y, x + cw - 6, y + rowH };
            if (tr.right > tr.left)
                cv.text(tr, r.cells[c], r.selected ? RGB(255, 255, 255) : pal.fore,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
            x += cw;
            if (x >= static_cast<int>(rc.right))
                break;
        }
        y += rowH;
    }
    cv.clipPop();

    // 表头：底色 + 列名 + 列分隔 + 底线
    if (headerH > 0)
    {
        RECT hr = { rc.left, rc.top, rc.right, rc.top + headerH };
        cv.fillRect(hr, pal.muted);
        cv.fillRect(RECT{ rc.left, hr.bottom - 1, rc.right, hr.bottom }, pal.border);
        int x = static_cast<int>(rc.left) + 4;
        for (size_t c = 0; c < header.size(); ++c)
        {
            const int cw = (c < widths.size() && widths[c] > 0)
                               ? widths[c]
                               : (static_cast<int>(rc.right) - x) / 4;
            RECT tr = { x, hr.top, x + cw - 6, hr.bottom };
            if (tr.right > tr.left)
                cv.text(tr, header[c], pal.fore,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
            x += cw;
            if (c + 1 < header.size() && x < static_cast<int>(rc.right))
                cv.strokeLine(POINT{ x - 3, hr.top + 3 }, POINT{ x - 3, hr.bottom - 3 },
                              pal.border, 1);
            if (x >= static_cast<int>(rc.right))
                break;
        }
    }

    if (hasFocus)
        cv.strokeRect(rc, pal.accent, 1);
}

// ---------------------------------------------------------------- 表头控件（SysHeader32）
inline void drawHeaderRow(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& titles,
                          const std::vector<int>& widths, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.muted);
    cv.fillRect(RECT{ rc.left, rc.bottom - 1, rc.right, rc.bottom }, pal.border);

    int x = static_cast<int>(rc.left) + 4;
    for (size_t c = 0; c < titles.size(); ++c)
    {
        const int cw = (c < widths.size() && widths[c] > 0)
                           ? widths[c]
                           : (static_cast<int>(rc.right) - x) / 4;
        RECT tr = { x, rc.top, x + cw - 6, rc.bottom };
        if (tr.right > tr.left)
            cv.text(tr, titles[c], pal.fore,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        x += cw;
        if (c + 1 < titles.size() && x < static_cast<int>(rc.right))
            cv.strokeLine(POINT{ x - 3, rc.top + 3 }, POINT{ x - 3, rc.bottom - 3 }, pal.border, 1);
        if (x >= static_cast<int>(rc.right))
            break;
    }
}

// ---------------------------------------------------------------- 日期选择框（SysDateTimePick32）
inline void drawDatePicker(XamlCanvas& cv, const RECT& rc, const std::wstring& text,
                           const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    RECT body = { rc.left, rc.top, rc.right, rc.bottom };
    cv.fillRoundRect(body, pal.back, pal.border, 3);

    RECT tr = { rc.left + 8, rc.top, rc.right - 26, rc.bottom };
    cv.text(tr, text, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
            pal.ptSize, pal.face);

    // 右侧日历小图标：一个方框 + 顶部横杠 + 两个日格
    const int bs = 13;
    const int bx = rc.right - bs - 7;
    const int by = (rc.top + rc.bottom - bs) / 2;
    RECT box = { bx, by, bx + bs, by + bs };
    cv.strokeRect(box, pal.fore, 1);
    cv.strokeLine(POINT{ bx, by + 4 }, POINT{ bx + bs, by + 4 }, pal.fore, 1);
    cv.fillRect(RECT{ bx + 3, by + 7, bx + 6, by + 10 }, pal.fore);
    cv.fillRect(RECT{ bx + 8, by + 7, bx + 11, by + 10 }, pal.fore);
}


// ---------------------------------------------------------------- 日历（SysMonthCal32）
// year/month：当前显示月；todayDay：今天的日（不在本月份时传 0）；selDay：选中日（0 = 未选）
inline void drawCalendar(XamlCanvas& cv, const RECT& rc, int year, int month, int todayDay,
                         int selDay, const Palette& pal, bool hasFocus = false)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top || month < 1 || month > 12)
        return;
    cv.fillRect(rc, pal.back);

    const int titleH = 26;
    const int headH = 20;
    const int w = rc.right - rc.left;

    // 标题条：yyyy年M月 + 左右箭头
    RECT tr = { rc.left, rc.top, rc.right, rc.top + titleH };
    cv.fillRect(tr, pal.muted);
    cv.fillRect(RECT{ tr.left, tr.bottom - 1, tr.right, tr.bottom }, pal.border);
    {
        wchar_t buf[64] = {};
        swprintf_s(buf, L"%d年%d月", year, month);
        RECT txt = { tr.left + 20, tr.top, tr.right - 20, tr.bottom };
        cv.text(txt, buf, pal.fore, DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        const int ay = (tr.top + tr.bottom) / 2;
        cv.strokeLine(POINT{ tr.left + 14, ay - 6 }, POINT{ tr.left + 8, ay }, pal.fore, 1);
        cv.strokeLine(POINT{ tr.left + 8, ay }, POINT{ tr.left + 14, ay + 6 }, pal.fore, 1);
        cv.strokeLine(POINT{ tr.right - 14, ay - 6 }, POINT{ tr.right - 8, ay }, pal.fore, 1);
        cv.strokeLine(POINT{ tr.right - 8, ay }, POINT{ tr.right - 14, ay + 6 }, pal.fore, 1);
    }

    // 星期表头（首列周日，与系统 MonthCal 一致）
    static const wchar_t* const kWeek[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
    const int cw = w / 7;
    const int headTop = rc.top + titleH;
    for (int i = 0; i < 7; ++i)
    {
        RECT cr = { rc.left + i * cw, headTop, rc.left + (i + 1) * cw, headTop + headH };
        cv.text(cr, kWeek[i], RGB(170, 180, 195),
                DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize - 1, pal.face);
    }

    // 当月 1 日是星期几（0 = 周日）
    SYSTEMTIME first = {};
    first.wYear = static_cast<WORD>(year);
    first.wMonth = static_cast<WORD>(month);
    first.wDay = 1;
    FILETIME ft = {};
    SYSTEMTIME norm = {};
    int firstDow = 0;
    if (::SystemTimeToFileTime(&first, &ft) && ::FileTimeToSystemTime(&ft, &norm))
        firstDow = static_cast<int>(norm.wDayOfWeek);

    static const int kDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int days = kDays[month - 1];
    if (month == 2)
    {
        const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (leap)
            days = 29;
    }

    const int gridTop = headTop + headH;
    const int cellH = (rc.bottom - gridTop) / 6;

    cv.clipPush(rc);
    for (int day = 1; day <= days; ++day)
    {
        const int idx = firstDow + day - 1;
        const int col = idx % 7;
        const int row = idx / 7;
        RECT cell = { rc.left + col * cw, gridTop + row * cellH,
                      rc.left + (col + 1) * cw, gridTop + (row + 1) * cellH };
        if (cell.bottom > rc.bottom)
            break;

        if (day == selDay)
        {
            RECT sel = { cell.left + 2, cell.top + 2, cell.right - 2, cell.bottom - 2 };
            cv.fillRoundRect(sel, pal.accent, pal.accent, 4);
        }
        else if (day == todayDay)
        {
            RECT sel = { cell.left + 2, cell.top + 2, cell.right - 2, cell.bottom - 2 };
            cv.strokeRect(sel, pal.accent, 1);
        }
        cv.text(cell, std::to_wstring(day),
                (day == selDay) ? RGB(255, 255, 255) : pal.fore,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
    }
    cv.clipPop();

    if (hasFocus)
        cv.strokeRect(rc, pal.accent, 1);
}

// 滚动条拇指矩形（命中测试用，坐标 = 控件客户区）
inline RECT scrollThumbRect(const RECT& rc, int pos, int minV, int maxV);

// ---------------------------------------------------------------- 滚动区域（ScrolledWindow）
// 自绘竖直滚动条：轨道 muted、滑块 accent；pos 在 [minV, maxV] 之间。
inline void drawScrollArea(XamlCanvas& cv, const RECT& rc, int pos, int minV, int maxV,
                           const Palette& pal, bool hasFocus = false)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.strokeRect(rc, pal.border, 1);

    if (maxV <= minV)
        return;

    const int barW = 10;
    RECT track = { rc.right - barW - 3, rc.top + 3, rc.right - 3, rc.bottom - 3 };
    const int trackH = track.bottom - track.top;
    if (trackH <= 0)
        return;
    cv.fillRoundRect(track, pal.muted, pal.muted, 4);

    const RECT thumb = scrollThumbRect(rc, pos, minV, maxV);
    if (thumb.bottom > thumb.top)
        cv.fillRoundRect(thumb, pal.accent, pal.accent, 4);

    if (hasFocus)
        cv.strokeRect(rc, pal.accent, 1);
}

// 滚动条拇指矩形（命中测试用，坐标 = 控件客户区）
inline RECT scrollThumbRect(const RECT& rc, int pos, int minV, int maxV)
{
    RECT empty = { 0, 0, 0, 0 };
    if (rc.right <= rc.left || rc.bottom <= rc.top || maxV <= minV)
        return empty;

    const int barW = 10;
    RECT track = { rc.right - barW - 3, rc.top + 3, rc.right - 3, rc.bottom - 3 };
    const int trackH = track.bottom - track.top;
    if (trackH <= 0)
        return empty;

    int thumbH = trackH / 4;
    if (thumbH < 24)
        thumbH = 24;
    if (thumbH > trackH)
        thumbH = trackH;

    const int span = maxV - minV;
    int p = pos - minV;
    if (p < 0)
        p = 0;
    if (p > span)
        p = span;
    const int freeH = trackH - thumbH;
    const int y = track.top + (span > 0 ? freeH * p / span : 0);
    RECT thumb = { track.left, y, track.right, y + thumbH };
    return thumb;
}

inline void drawTree(XamlCanvas& cv, const RECT& rc, const std::vector<xamlctl::TreeRow>& rows,
                     COLORREF back, COLORREF fore, COLORREF accent, int scrollY = 0,
                     int ptSize = 9, const std::wstring& face = L"Segoe UI")
{
    xamlctl::TreeStyle st;
    st.back = back;
    st.fore = fore;
    st.selBack = accent;
    st.selFore = RGB(255, 255, 255);
    st.guide = RGB(120, 128, 140);
    st.rowH = 24;
    xamlctl::xamlDrawTree(cv, rc, rows, st, scrollY, ptSize, face);
}

// 文本编辑控件（Edit / RichText）的一显示行。
// 像素量（选区起止、光标位置）都由调用方用控件自身的字体量好后传进来，
// 这里只负责画：测量归 Win32，绘制归 D2D，两边用同一份字体才不会让光标和字错位。
struct EditLine
{
    std::wstring text;
    int x = 0;          // 这一行文字的起始 x（客户区坐标，水平滚动时为负）
    int y = 0;          // 这一行的顶边（客户区坐标）
    int selFromPx = -1; // 行内选区左边界（相对的 x，-1 表示这行没有选区）
    int selToPx = -1;
    int caretPx = -1;   // 光标竖线所在的 x（相对行内，-1 表示光标不在这行）
};

// 文本编辑控件：底板 + 边框 + 选区 + 文字 + 光标
inline void drawEdit(XamlCanvas& cv, const RECT& rc, const std::vector<EditLine>& lines,
                     const Palette& pal, bool hasFocus, bool readonly, int lineH,
                     UINT align = DT_LEFT, bool showCaret = true)
{
    cv.fillRect(rc, pal.back);
    cv.strokeRect(rc, hasFocus ? pal.accent : pal.border, 1);

    // 焦点内发光：贴着边框的一圈半透明强调色。这条同时是 alpha 管线的探针 ——
    // 真混合的话像素是"底色 + 强调色"的中间值，不是纯强调色也不是纯底色。
    if (hasFocus)
    {
        RECT g = rc;
        ::InflateRect(&g, -1, -1);
        if (g.right > g.left && g.bottom > g.top)
        {
            cv.fillRectAlpha(RECT{ g.left, g.top, g.right, g.top + 2 }, pal.accent, 70);
            cv.fillRectAlpha(RECT{ g.left, g.top, g.left + 2, g.bottom }, pal.accent, 40);
        }
    }

    for (const EditLine& ln : lines)    {
        RECT rowRc = { rc.left, ln.y, rc.right, ln.y + lineH };
        if (rowRc.bottom > rc.bottom - 1)
            rowRc.bottom = rc.bottom - 1;
        if (rowRc.top < rc.top + 1)
            rowRc.top = rc.top + 1;
        if (rowRc.bottom <= rowRc.top)
            continue;

        // 选区先铺、文字后画：文字始终压在选区色上，选中时仍然看得清
        if (ln.selToPx > ln.selFromPx)
        {
            RECT sr = { ln.x + ln.selFromPx, rowRc.top, ln.x + ln.selToPx, rowRc.bottom };
            if (sr.left < rc.left + 1)
                sr.left = rc.left + 1;
            if (sr.right > rc.right - 1)
                sr.right = rc.right - 1;
            if (sr.right > sr.left)
                cv.fillRect(sr, pal.accent);
        }

        if (!ln.text.empty())
        {
            RECT tr = { ln.x, rowRc.top, rc.right - 2, rowRc.bottom };
            if (align == DT_CENTER || align == DT_RIGHT)
            {
                tr.left = rc.left + 2;
                tr.right = rc.right - 2;
            }
            const COLORREF fg = readonly ? RGB(170, 176, 184) : pal.fore;
            cv.text(tr, ln.text, fg, align, pal.ptSize, pal.face);
        }

        // 光标：只在有焦点且闪烁的"亮"相位画，整块重画所以不会留残迹
        if (hasFocus && showCaret && ln.caretPx >= 0)
        {
            const int cx = ln.x + ln.caretPx;
            if (cx >= rc.left + 1 && cx < rc.right - 1)
            {
                RECT cr = { cx, rowRc.top, cx + 1, rowRc.bottom };
                cv.fillRect(cr, pal.fore);
            }
        }
    }
}

// ---------------------------------------------------------------- QML 对齐：Switch
// 轨道 + 滑块，开=强调色实心轨道，关=muted 轨道。整块重画，不做动画（后续接 AnimationHost）。
inline void drawSwitch(XamlCanvas& cv, const RECT& rc, bool on, const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    const int h = rc.bottom - rc.top;
    const int trackH = h >= 24 ? 22 : h;
    const int trackW = trackH * 2 > (rc.right - rc.left) ? (rc.right - rc.left) : trackH * 2;
    RECT track = { rc.left, (rc.top + rc.bottom - trackH) / 2, rc.left + trackW,
                   (rc.top + rc.bottom + trackH) / 2 };
    cv.fillRoundRect(track, on ? pal.accent : pal.muted, on ? pal.accent : pal.border, trackH / 2);

    const int knobD = trackH - 4;
    const int kcy = (track.top + track.bottom) / 2;
    int kcx = on ? (track.right - 2 - knobD / 2) : (track.left + 2 + knobD / 2);
    RECT knob = { kcx - knobD / 2, kcy - knobD / 2, kcx + knobD / 2, kcy + knobD / 2 };
    cv.fillRoundRect(knob, RGB(245, 247, 250), RGB(245, 247, 250), knobD / 2);

    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

// ---------------------------------------------------------------- QML 对齐：SpinBox
// 左侧数字、右侧上下两个小三角按钮，点三角或滚轮改值。
inline void drawSpinBox(XamlCanvas& cv, const RECT& rc, int value, const Palette& pal, bool focus,
                        bool readOnly = false)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.fillRoundRect(rc, pal.muted, focus ? pal.accent : pal.border, 3);

    const int btnW = 18;
    RECT face = { rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1 };
    cv.fillRoundRect(face, pal.back, pal.back, 2);
    RECT textRc = { face.left + 6, face.top, face.right - btnW - 2, face.bottom };
    wchar_t buf[32] = {};
    swprintf_s(buf, 32, L"%d", value);
    cv.text(textRc, buf, readOnly ? RGB(170, 176, 184) : pal.fore,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);

    // 上下两个三角
    const int cx = face.right - btnW / 2;
    const int cyTop = (face.top + face.bottom) / 2;
    for (int k = 0; k < 2; ++k)
    {
        const int y0 = k == 0 ? face.top + 3 : cyTop + 2;
        const int y1 = k == 0 ? cyTop - 2 : face.bottom - 3;
        POINT a = { cx - 4, k == 0 ? y0 : y1 };
        POINT b = { cx + 4, k == 0 ? y0 : y1 };
        POINT c = { cx, k == 0 ? y1 : y0 };
        cv.strokeLine(a, b, pal.fore, 1);
        cv.strokeLine(a, c, pal.fore, 1);
        cv.strokeLine(b, c, pal.fore, 1);
    }
}

// ---------------------------------------------------------------- QML 对齐：RangeSlider
// 双拇指：中间区间用强调色，两端 muted。两个拇指都带描边。
inline void drawRangeSlider(XamlCanvas& cv, const RECT& rc, int lo, int hi, int vmin, int vmax,
                            const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    const int span = vmax > vmin ? vmax - vmin : 0;
    const int knob = 14;
    const int cy = (rc.top + rc.bottom) / 2;
    const int usable = (rc.right - knob) - rc.left;

    auto posOf = [&](int v) -> int {
        if (span <= 0) return rc.left + knob / 2;
        return rc.left + knob / 2 +
               static_cast<int>((static_cast<long long>(usable) * (v - vmin)) / span);
    };

    RECT track = { rc.left + knob / 2, cy - 2, rc.right - knob / 2, cy + 2 };
    if (track.right > track.left)
        cv.fillRoundRect(track, pal.muted, pal.border, 2);

    const int kLo = posOf(lo);
    const int kHi = posOf(hi);
    if (kHi > kLo)
    {
        RECT fill = { kLo, cy - 2, kHi, cy + 2 };
        cv.fillRoundRect(fill, pal.accent, pal.accent, 2);
    }
    for (int kx : {kLo, kHi})
    {
        RECT kr = { kx - knob / 2, cy - knob / 2, kx + knob / 2, cy + knob / 2 };
        cv.fillRoundRect(kr, pal.accent, pal.border, 3);
    }
}

// ---------------------------------------------------------------- QML 对齐：BusyIndicator
// 12 段的旋转环：亮段集中在一处，phase 每次重画 +1。
inline void drawBusyRing(XamlCanvas& cv, const RECT& rc, int phase, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    const int sz = (rc.right - rc.left) < (rc.bottom - rc.top) ? (rc.right - rc.left)
                                                              : (rc.bottom - rc.top);
    const int cx = (rc.left + rc.right) / 2;
    const int cy = (rc.top + rc.bottom) / 2;
    const int r = sz / 2 - 3;
    const int segs = 12;

    // 用 12 个点近似圆：角度均匀分布，亮度按相位决定（越靠近相位头越亮）
    const double kPi = 3.14159265358979323846;
    for (int i = 0; i < segs; ++i)
    {
        const double ang = (2.0 * kPi * i) / segs;
        const int px = cx + static_cast<int>(r * 0.86 * std::cos(ang));
        const int py = cy + static_cast<int>(r * 0.86 * std::sin(ang));
        const int d = (i - (phase % segs) + segs) % segs;
        const int alpha = d < segs / 2 ? 255 - d * 36 : 45;
        cv.fillRectAlpha({ px - 2, py - 2, px + 2, py + 2 }, pal.accent, alpha);
    }
}

// ---------------------------------------------------------------- QML 对齐：PageIndicator
// 一排小圆点，当前页是长条。
inline void drawPageDots(XamlCanvas& cv, const RECT& rc, int count, int cur, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    if (count <= 0)
        count = 1;

    const int cy = (rc.top + rc.bottom) / 2;
    const int dot = 8;
    const int gap = 6;
    const int totalW = count * dot + (count - 1) * gap;
    int x = (rc.left + rc.right - totalW) / 2;
    for (int i = 0; i < count; ++i)
    {
        RECT r = { x, cy - dot / 2, x + dot, cy + dot / 2 };
        if (i == cur)
            cv.fillRoundRect(r, pal.accent, pal.accent, dot / 2);
        else
            cv.fillRoundRect(r, pal.muted, pal.border, dot / 2);
        x += dot + gap;
    }
}

// ---------------------------------------------------------------- QML 对齐：Dial
// 270° 圆弧（135° 起、405° 止）+ 指针 + 中心数值。点/拖都按角度取值。
inline void drawDial(XamlCanvas& cv, const RECT& rc, int value, int vmin, int vmax,
                     const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    const int cx = (rc.left + rc.right) / 2;
    const int cy = (rc.top + rc.bottom) / 2;
    int r = (rc.right - rc.left) < (rc.bottom - rc.top) ? (rc.right - rc.left) / 2
                                                       : (rc.bottom - rc.top) / 2;
    r -= 3;
    if (r < 8)
        return;

    const double kPi = 3.14159265358979323846;
    const double start = 135.0 * kPi / 180.0;      // 左下
    const double sweep = 270.0 * kPi / 180.0;      // 顺时针一圈少 90°
    const int span = vmax > vmin ? vmax - vmin : 0;
    const int filled = span > 0 ? static_cast<int>(((value - vmin) * 36LL) / span) : 0;

    // 弧线用 36 个小圆点铺出来（D2D 没有 arc 原语）
    for (int i = 0; i <= 36; ++i)
    {
        const double a = start + (sweep * i) / 36.0;
        const int px = cx + static_cast<int>(r * std::cos(a));
        const int py = cy + static_cast<int>(r * std::sin(a));
        RECT d = { px - 2, py - 2, px + 2, py + 2 };
        const bool on = i <= filled;
        cv.fillEllipse(d, on ? pal.accent : pal.muted, on ? pal.accent : pal.border);
    }

    // 指针
    const double pa = start + (span > 0 ? (sweep * (value - vmin)) / span : 0.0);
    POINT from = { cx + static_cast<int>((r - 9) * std::cos(pa)),
                   cy + static_cast<int>((r - 9) * std::sin(pa)) };
    POINT to = { cx, cy };
    cv.strokeLine(from, to, pal.fore, 2);

    RECT hub = { cx - 3, cy - 3, cx + 3, cy + 3 };
    cv.fillEllipse(hub, pal.accent, pal.accent);

    wchar_t buf[32] = {};
    swprintf_s(buf, 32, L"%d", value);
    RECT tr = { cx - 18, cy + 8, cx + 18, cy + 26 };
    cv.text(tr, buf, pal.fore, DT_CENTER | DT_TOP | DT_SINGLELINE, pal.ptSize, pal.face);

    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 4, 1);
}

// ---------------------------------------------------------------- QML 对齐：Tumbler
// 三行滚动选择：中间是高亮当前项，上下各一行相邻项（淡）。
inline void drawTumbler(XamlCanvas& cv, const RECT& rc, int index, int count,
                        const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    if (count <= 0)
        count = 10;
    cv.fillRoundRect(rc, pal.muted, focus ? pal.accent : pal.border, 4);

    const int h = rc.bottom - rc.top;
    const int rowH = h / 3 > 0 ? h / 3 : h;
    RECT mid = { rc.left + 1, rc.top + rowH, rc.right - 1, rc.top + 2 * rowH };
    cv.fillRect(mid, pal.back);
    cv.fillRoundRect({ mid.left, mid.top, mid.right, mid.top + 2 }, pal.accent, pal.accent, 1);
    cv.fillRoundRect({ mid.left, mid.bottom - 2, mid.right, mid.bottom }, pal.accent, pal.accent, 1);

    wchar_t buf[32] = {};
    const int prev = (index - 1 + count) % count;
    const int next = (index + 1) % count;

    swprintf_s(buf, 32, L"%d", prev);
    RECT r0 = { rc.left + 1, rc.top + 1, rc.right - 1, rc.top + rowH };
    cv.text(r0, buf, RGB(150, 157, 166), DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            pal.ptSize, pal.face);

    swprintf_s(buf, 32, L"%d", index);
    cv.text(mid, buf, pal.fore, DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize + 2, pal.face);

    swprintf_s(buf, 32, L"%d", next);
    RECT r2 = { rc.left + 1, rc.top + 2 * rowH, rc.right - 1, rc.bottom - 1 };
    cv.text(r2, buf, RGB(150, 157, 166), DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            pal.ptSize, pal.face);
}

// ---------------------------------------------------------------- QML 对齐：独立 ScrollBar
// 可单独摆放的滚动条：轨道 + 拇指，拇指长度按 page/span 算。
inline void drawScrollBar(XamlCanvas& cv, const RECT& rc, int pos, int vmin, int vmax, int page,
                          bool vertical, const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);

    const int span = vmax > vmin ? vmax - vmin : 0;
    if (page <= 0)
        page = 1;

    if (vertical)
    {
        const int w = rc.right - rc.left;
        RECT track = { rc.left + w / 2 - 2, rc.top, rc.left + w / 2 + 2, rc.bottom };
        cv.fillRoundRect(track, pal.muted, pal.border, 2);

        const int total = rc.bottom - rc.top;
        int thumbH = static_cast<int>((static_cast<long long>(total) * page) / (span + page));
        if (thumbH < 12)
            thumbH = 12;
        if (thumbH > total)
            thumbH = total;
        const int free = total - thumbH;
        const int ty = rc.top + (span > 0 ? static_cast<int>((static_cast<long long>(free) * (pos - vmin)) / span) : 0);
        RECT thumb = { rc.left + 1, ty, rc.right - 1, ty + thumbH };
        cv.fillRoundRect(thumb, pal.accent, focus ? pal.accent : pal.border, 3);
    }
    else
    {
        const int h = rc.bottom - rc.top;
        RECT track = { rc.left, rc.top + h / 2 - 2, rc.right, rc.top + h / 2 + 2 };
        cv.fillRoundRect(track, pal.muted, pal.border, 2);

        const int total = rc.right - rc.left;
        int thumbW = static_cast<int>((static_cast<long long>(total) * page) / (span + page));
        if (thumbW < 12)
            thumbW = 12;
        if (thumbW > total)
            thumbW = total;
        const int free = total - thumbW;
        const int tx = rc.left + (span > 0 ? static_cast<int>((static_cast<long long>(free) * (pos - vmin)) / span) : 0);
        RECT thumb = { tx, rc.top + 1, tx + thumbW, rc.bottom - 1 };
        cv.fillRoundRect(thumb, pal.accent, focus ? pal.accent : pal.border, 3);
    }

    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

// ---------------------------------------------------------------- QML 对齐：TableView
// 表头 + 数据行 + 选中行 + 斑马纹 + 滚动，列宽平均分。点表头排序由宿主改变数据顺序。
inline void drawTable(XamlCanvas& cv, const RECT& rc,
                      const std::vector<std::wstring>& cols,
                      const std::vector<std::vector<std::wstring>>& rows,
                      int selRow, int scrollRow, int sortCol, bool sortAsc,
                      const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    if (cols.empty())
        return;

    const int headH = 26;
    const int rowH = 24;
    const int ncol = static_cast<int>(cols.size());
    const int cw = (rc.right - rc.left) / ncol;

    cv.clipPush(rc);

    // 表头
    RECT hd = { rc.left, rc.top, rc.right, rc.top + headH };
    cv.fillRect(hd, pal.muted);
    for (int c = 0; c < ncol; ++c)
    {
        RECT cr = { rc.left + c * cw + 6, hd.top, rc.left + (c + 1) * cw - 4, hd.bottom };
        std::wstring t = cols[c];
        if (c == sortCol)
            t += sortAsc ? L" ▲" : L" ▼";
        cv.text(cr, t, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        if (c > 0)
            cv.strokeLine({ rc.left + c * cw, hd.top }, { rc.left + c * cw, hd.bottom }, pal.border, 1);
    }
    cv.strokeLine({ rc.left, hd.bottom }, { rc.right, hd.bottom }, pal.border, 1);

    // 数据行
    const int bodyTop = hd.bottom + 1;
    const int visibleRows = (rc.bottom - bodyTop) / rowH;
    const int total = static_cast<int>(rows.size());
    for (int i = 0; i < visibleRows; ++i)
    {
        const int r = scrollRow + i;
        if (r >= total)
            break;
        RECT rr = { rc.left, bodyTop + i * rowH, rc.right, bodyTop + (i + 1) * rowH };
        if (r == selRow)
            cv.fillRectAlpha(rr, pal.accent, 70);
        else if (i % 2 == 1)
            cv.fillRect(rr, pal.muted);

        const std::vector<std::wstring>& line = rows[r];
        for (int c = 0; c < ncol; ++c)
        {
            RECT cr = { rc.left + c * cw + 6, rr.top, rc.left + (c + 1) * cw - 4, rr.bottom };
            const std::wstring cell = c < static_cast<int>(line.size()) ? line[c] : L"";
            cv.text(cr, cell, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
            if (c > 0)
                cv.strokeLine({ rc.left + c * cw, rr.top }, { rc.left + c * cw, rr.bottom }, pal.border, 1);
        }
    }
    cv.clipPop();

    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

// ---------------------------------------------------------------- QML 对齐：TreeView
// 层级表格：缩进 + 展开箭头 + 选中行；visible 由宿主按折叠状态算好传进来。
inline void drawTreeTable(XamlCanvas& cv, const RECT& rc,
                          const std::vector<std::wstring>& texts,
                          const std::vector<int>& levels,
                          const std::vector<bool>& hasKids,
                          const std::vector<bool>& collapsed,
                          int selRow, int scrollRow, const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.clipPush(rc);

    const int rowH = 24;
    const int visibleRows = (rc.bottom - rc.top) / rowH;
    for (int i = 0; i < visibleRows; ++i)
    {
        const int r = scrollRow + i;
        if (r >= static_cast<int>(texts.size()))
            break;
        RECT rr = { rc.left, rc.top + i * rowH, rc.right, rc.top + (i + 1) * rowH };
        if (r == selRow)
            cv.fillRectAlpha(rr, pal.accent, 70);

        const int indent = rc.left + 6 + levels[r] * 16;
        if (hasKids[r])
        {
            // 折叠是 ▸，展开是 ▾
            POINT a, b, c2;
            if (collapsed[r])
            {
                a = { indent, rr.top + 6 };
                b = { indent, rr.bottom - 6 };
                c2 = { indent + 7, (rr.top + rr.bottom) / 2 };
            }
            else
            {
                a = { indent, rr.top + 9 };
                b = { indent + 8, rr.top + 9 };
                c2 = { indent + 4, rr.bottom - 6 };
            }
            cv.strokeLine(a, b, pal.fore, 1);
            cv.strokeLine(b, c2, pal.fore, 1);
            cv.strokeLine(c2, a, pal.fore, 1);
        }
        RECT tr = { indent + 12, rr.top, rc.right - 4, rr.bottom };
        cv.text(tr, texts[r], pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
    }
    cv.clipPop();

    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

// ---------------------------------------------------------------- QML 对齐：ToolBar
// 一排工具按钮：选中/悬停用强调色底，条目之间一条细竖线分隔。
inline void drawToolBar(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& items,
                        int sel, int hover, const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.strokeLine({ rc.left, rc.bottom - 1 }, { rc.right, rc.bottom - 1 }, pal.border, 1);

    const int btnW = 74;
    const int btnH = (rc.bottom - rc.top) - 8;
    int x = rc.left + 4;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        RECT b = { x, rc.top + 4, x + btnW, rc.top + 4 + btnH };
        if (b.right > rc.right)
            break;
        if (i == sel)
            cv.fillRoundRect(b, pal.accent, pal.accent, 4);
        else if (i == hover)
            cv.fillRoundRect(b, pal.muted, pal.border, 4);
        cv.text(b, items[i], i == sel ? RGB(255, 255, 255) : pal.fore,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        x += btnW + 4;
    }
    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

// ---------------- 阴影底板（HTML box-shadow 的等价物） ----------------
// D2D 画布只给纯色填充、没有可用的 alpha 图层，所以把阴影透明度预先混到"背后那层底色"上，
// 再逐层向外扩 1px 逼近模糊：视觉上就是 CSS 的 box-shadow: 0 4px 12px rgba(16,24,40,.1)。
struct ShadowBoxStyle
{
    int pad = 12;                       // 窗口比视觉矩形外扩多少（留给阴影）
    int radius = 6;
    int dx = 0, dy = 4;
    int blur = 12;
    int alpha = 26;                     // 0..255
    COLORREF shadow = RGB(16, 24, 40);
    COLORREF bg = RGB(255, 255, 255);   // 阴影落在什么底色上（用于预混）
    COLORREF back = RGB(255, 255, 255);
    COLORREF border = RGB(208, 213, 221);
    int borderW = 1;
};

inline COLORREF mixColor(COLORREF fg, COLORREF bg, int opacity)
{
    if (opacity >= 255) return fg;
    if (opacity <= 0) return bg;
    const int a = opacity, b = 255 - opacity;
    return RGB((GetRValue(fg) * a + GetRValue(bg) * b) / 255,
               (GetGValue(fg) * a + GetGValue(bg) * b) / 255,
               (GetBValue(fg) * a + GetBValue(bg) * b) / 255);
}

inline void drawShadowBox(XamlCanvas& cv, const RECT& win, const ShadowBoxStyle& s)
{
    RECT vis = { win.left + s.pad, win.top + s.pad, win.right - s.pad, win.bottom - s.pad };
    if (vis.right <= vis.left || vis.bottom <= vis.top)
        return;
    // 这个 skin 窗口本身就是"阴影底板"，pad 区必须先用背后底色铺满，
    // 否则没画到的像素会留成画布初值（黑），看起来像给矩形描了黑框。
    cv.fillRect(win, s.bg);
    const int r0 = s.radius > 0 ? s.radius : 0;
    if (s.blur > 0 && s.alpha > 0)
    {
        for (int i = s.blur; i >= 1; --i)
        {
            const int k = s.blur - i + 1;                        // 越靠内越浓
            const int a = s.alpha * k * k / (s.blur * s.blur);
            if (a <= 0) continue;
            RECT r = vis;
            ::OffsetRect(&r, s.dx, s.dy);
            ::InflateRect(&r, i, i);
            const COLORREF c = mixColor(s.shadow, s.bg, a);
            cv.fillRoundRect(r, c, c, r0 + i);
        }
    }
    cv.fillRoundRect(vis, s.back, s.back, r0);
    if (s.borderW > 0)
        cv.strokeRoundRect(vis, s.border, r0, s.borderW);
}

// ---------------- 浮层族：ToolTip / Menu / MenuBar / Popup ----------------
inline void drawToolTip(XamlCanvas& cv, const RECT& rc, const std::wstring& text,
                        const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRoundRect(rc, pal.muted, pal.border, 4);
    RECT in = rc;
    InflateRect(&in, -1, -1);
    cv.strokeRoundRect(in, pal.border, 4, 1);
    RECT tx = { rc.left + 8, rc.top, rc.right - 8, rc.bottom };
    cv.text(tx, text, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
}

// items 里写 "-" 表示一条分隔线；hover/sel 是行下标
inline void drawMenu(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& items,
                     int hover, int sel, const Palette& pal, bool focus)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    // 菜单浮层本体要比页面背景亮一档，否则整块菜单和背景同色、只看得到文字
    const COLORREF surface = RGB(58, 67, 79);
    cv.fillRect(rc, surface);
    cv.strokeRoundRect(rc, pal.border, 3, 1);
    const int rowH = 26;
    int y = rc.top + 4;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        RECT r = { rc.left + 4, y, rc.right - 4, y + rowH };
        if (r.bottom > rc.bottom)
            break;
        if (items[i] == L"-")
        {
            cv.strokeLine({ r.left + 6, y + rowH / 2 }, { r.right - 6, y + rowH / 2 }, pal.border, 1);
            y += 8;
            continue;
        }
        if (i == sel)
            cv.fillRoundRect(r, pal.accent, pal.accent, 3);
        else if (i == hover)
            cv.fillRoundRect(r, pal.muted, pal.border, 3);
        RECT tx = { r.left + 10, r.top, r.right - 6, r.bottom };
        cv.text(tx, items[i], i == sel ? RGB(255, 255, 255) : pal.fore,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        y += rowH;
    }
    if (focus)
        cv.strokeRoundRect(rc, pal.accent, 3, 1);
}

inline void drawMenuBar(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& items,
                        int sel, int hover, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    cv.strokeLine({ rc.left, rc.bottom - 1 }, { rc.right, rc.bottom - 1 }, pal.border, 1);
    const int cellW = 88;
    int x = rc.left + 2;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        RECT r = { x, rc.top, x + cellW, rc.bottom - 1 };
        if (r.right > rc.right)
            break;
        if (i == sel || i == hover)
            cv.fillRect(r, i == sel ? pal.accent : pal.muted);
        cv.text(r, items[i], i == sel ? RGB(255, 255, 255) : pal.fore,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        x += cellW;
    }
}

inline void drawPopup(XamlCanvas& cv, const RECT& rc, const std::wstring& title,
                      const std::vector<std::wstring>& lines, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    const COLORREF surface = RGB(58, 67, 79);
    cv.fillRoundRect(rc, surface, pal.border, 6);
    RECT in = rc;
    InflateRect(&in, -1, -1);
    cv.strokeRoundRect(in, pal.border, 6, 1);
    RECT th = { rc.left + 1, rc.top + 1, rc.right - 1, rc.top + 33 };
    cv.fillRect(th, pal.muted);
    RECT tt = { th.left + 10, th.top, th.right - 10, th.bottom };
    cv.text(tt, title, pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
    cv.strokeLine({ rc.left + 1, rc.top + 33 }, { rc.right - 1, rc.top + 33 }, pal.border, 1);
    int y = rc.top + 40;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        RECT r = { rc.left + 12, y, rc.right - 12, y + 22 };
        if (r.bottom > rc.bottom - 4)
            break;
        cv.text(r, lines[i], pal.fore, DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
        y += 22;
    }
}

// 虚拟化列表：只画"可见窗口内"的行，行高固定，委托是内置模板
// （标题 + 副标题 + 右侧值），不为每行建 HWND。
// vT/vS/vV 是截出来的可见段，first 是这段第一行在数据里的绝对下标，dy 是滚动余量。
inline void drawListView(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& vT,
                         const std::vector<std::wstring>& vS, const std::vector<std::wstring>& vV,
                         int first, int sel, int hover, int dy, int itemH,
                         int scrollPos, int scrollMax, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    const int rowH = itemH > 8 ? itemH : 44;
    int y = rc.top - dy;
    for (size_t i = 0; i < vT.size(); ++i)
    {
        RECT r = { rc.left, y, rc.right, y + rowH };
        if (r.bottom > rc.top && r.top < rc.bottom)
        {
            const int abs = first + static_cast<int>(i);
            if (abs == sel)
                cv.fillRectAlpha(r, pal.accent, 70);
            else if (abs == hover)
                cv.fillRect(r, pal.muted);
            RECT t1 = { r.left + 12, r.top + 6, r.right - 12, r.top + 6 + rowH / 2 };
            RECT t2 = { r.left + 12, r.top + rowH / 2, r.right - 90, r.bottom - 2 };
            RECT t3 = { r.right - 96, r.top, r.right - 12, r.bottom };
            cv.text(t1, vT[i], abs == sel ? RGB(255, 255, 255) : pal.fore,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
            if (i < vS.size() && !vS[i].empty())
                cv.text(t2, vS[i], abs == sel ? RGB(230, 236, 244) : RGB(150, 160, 172),
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize - 1, pal.face);
            if (i < vV.size() && !vV[i].empty())
                cv.text(t3, vV[i], abs == sel ? RGB(255, 255, 255) : pal.fore,
                        DT_RIGHT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
            if (r.bottom > rc.top && r.bottom < rc.bottom)
                cv.strokeLine({ rc.left + 8, r.bottom }, { rc.right - 8, r.bottom }, RGB(72, 81, 94), 1);
        }
        y += rowH;
    }
    // 右侧细滚动条：拇指长度按"可见行/总行"估
    if (scrollMax > 0)
    {
        RECT track = { rc.right - 6, rc.top + 2, rc.right - 2, rc.bottom - 2 };
        cv.fillRect(track, RGB(44, 51, 61));
        const int trackH = track.bottom - track.top;
        const int pageRows = (rc.bottom - rc.top) / rowH;
        const int totalRows = scrollMax / rowH + pageRows;
        int thumbH = totalRows > 0 ? trackH * pageRows / totalRows : trackH;
        if (thumbH < 18) thumbH = 18;
        const int off = trackH - thumbH;
        int top = track.top + (off > 0 ? off * scrollPos / scrollMax : 0);
        if (top + thumbH > track.bottom) top = track.bottom - thumbH;
        RECT thumb = { track.left, top, track.right, top + thumbH };
        cv.fillRoundRect(thumb, pal.border, pal.border, 2);
    }
}

// 页容器（StackView / SwipeView / Page）：底部 26px 页码条 + 页面底板。
// 页里的真实子控件由宿主整组平移，这里只画"容器自己"的那一层。
inline void drawPager(XamlCanvas& cv, const RECT& rc, int index, int count, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    if (count <= 0)
        return;
    RECT foot = { rc.left, rc.bottom - 26, rc.right, rc.bottom };
    cv.fillRect(foot, RGB(44, 51, 61));
    cv.strokeLine({ foot.left, foot.top }, { foot.right, foot.top }, RGB(72, 81, 94), 1);
    const int cy = foot.top + 13;
    const int gap = 16;
    int x = (foot.left + foot.right - (count - 1) * gap) / 2;
    for (int i = 0; i < count; ++i)
    {
        RECT dot = { x - 4, cy - 4, x + 4, cy + 4 };
        if (i == index)
            cv.fillRoundRect(dot, pal.accent, pal.accent, 4);
        else
            cv.fillRoundRect(dot, RGB(120, 132, 146), RGB(120, 132, 146), 4);
        x += gap;
    }
    wchar_t buf[32];
    swprintf_s(buf, L"%d / %d", index + 1, count);
    RECT tr = { foot.right - 60, foot.top, foot.right - 8, foot.bottom };
    cv.text(tr, buf, RGB(170, 180, 192), DT_RIGHT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
}


// ===========================================================================
// GridView：网格化委托（单元格可选、悬停、虚拟化只画可见行）
// 与 drawListView 共用同一套数据槽位：vT 是可见段的标题，first 是这段第一格
// 在数据里的绝对下标；dy 是滚动余量（按行整除后的余数）。
// ===========================================================================
inline void drawGridView(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& vT,
                         const std::vector<std::wstring>& vS, int first, int sel, int hover,
                         int dy, int cellW, int cellH, int cols,
                         int scrollPos, int scrollMax, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    const int cw = cellW > 24 ? cellW : 88;
    const int ch = cellH > 24 ? cellH : 88;
    const int n = cols > 0 ? cols : 2;
    const int pad = 4;
    for (size_t i = 0; i < vT.size(); ++i)
    {
        const int abs = first + static_cast<int>(i);
        const int col = static_cast<int>(i) % n;
        const int row = static_cast<int>(i) / n;
        RECT cell = { rc.left + col * cw + pad, rc.top + row * ch - dy + pad,
                      rc.left + (col + 1) * cw - pad, rc.top + (row + 1) * ch - dy - pad };
        if (cell.bottom <= rc.top || cell.top >= rc.bottom)
            continue;
        COLORREF fill = abs == sel ? pal.accent : (abs == hover ? pal.muted : RGB(58, 67, 79));
        cv.fillRoundRect(cell, fill, RGB(72, 81, 94), 6);
        RECT t1 = { cell.left + 6, cell.top + 6, cell.right - 6, cell.top + ch / 2 };
        RECT t2 = { cell.left + 6, cell.top + ch / 2, cell.right - 6, cell.bottom - 4 };
        cv.text(t1, vT[i], abs == sel ? RGB(255, 255, 255) : pal.fore,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        if (i < vS.size() && !vS[i].empty())
            cv.text(t2, vS[i], abs == sel ? RGB(230, 236, 244) : RGB(150, 160, 172),
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize - 1, pal.face);
    }

    // 右侧细滚动条（和列表同一套观感）
    if (scrollMax > 0)
    {
        const int trackH = rc.bottom - rc.top;
        const int thumbH0 = trackH * trackH / (trackH + scrollMax);
        const int thumbH = thumbH0 > 20 ? thumbH0 : 20;
        const int thumbY = rc.top + (trackH - thumbH) * scrollPos / scrollMax;
        RECT tr = { rc.right - 4, rc.top, rc.right, rc.bottom };
        cv.fillRect(tr, RGB(44, 51, 61));
        RECT th = { rc.right - 4, thumbY, rc.right, thumbY + thumbH };
        cv.fillRoundRect(th, RGB(120, 132, 146), RGB(120, 132, 146), 2);
    }
}

// ===========================================================================
// PathView：沿一条水平弧线摆卡片，选中项居中并放大，两侧按参数收敛。
// 路径用抛物线近似（中心最高、两端下垂），不需要真的贝塞尔求值器。
// ===========================================================================
inline void drawPathView(XamlCanvas& cv, const RECT& rc, const std::vector<std::wstring>& items,
                         int index, const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    const int count = static_cast<int>(items.size());
    if (count <= 0)
        return;
    const int cx = (rc.left + rc.right) / 2;
    const int baseY = rc.top + (rc.bottom - rc.top) * 3 / 4;
    const int amp = (rc.bottom - rc.top) / 6;
    const int step = 92;
    const int cardW = 76;
    for (int k = -2; k <= 2; ++k)
    {
        const int i = index + k;
        if (i < 0 || i >= count)
            continue;
        // 越靠中心越高、越大；两侧收缩并用透明度区分远近
        const int dx = k * step;
        const double t = 1.0 - (k * k) * 0.16;          // 1.0 / 0.84 / 0.36
        const int w = static_cast<int>(cardW * (0.72 + 0.28 * t));
        const int h = static_cast<int>(cardW * (0.72 + 0.28 * t));
        const int y = baseY - amp - static_cast<int>((amp + 28) * (1.0 - t));
        RECT card = { cx + dx - w / 2, y - h, cx + dx + w / 2, y };
        if (k == 0)
        {
            cv.fillRoundRect(card, pal.accent, RGB(0, 90, 160), 8);
        }
        else
        {
            cv.fillRoundRect(card, RGB(58, 67, 79), RGB(72, 81, 94), 8);
        }
        RECT tr = { card.left + 4, card.top, card.right - 4, card.bottom };
        cv.text(tr, items[i], k == 0 ? RGB(255, 255, 255) : RGB(200, 208, 220),
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, pal.ptSize, pal.face);
        // 卡片根部到基线的短腿，强调"挂在路径上"
        cv.strokeLine({ cx + dx, card.bottom }, { cx + dx, baseY }, RGB(72, 81, 94), 1);
    }
    wchar_t buf[48];
    swprintf_s(buf, L"%d / %d", index + 1, count);
    RECT foot = { rc.left + 8, rc.bottom - 24, rc.right - 8, rc.bottom - 4 };
    cv.text(foot, buf, RGB(150, 160, 172), DT_LEFT | DT_VCENTER | DT_SINGLELINE, pal.ptSize, pal.face);
}

// ===========================================================================
// QCanvas：QML Canvas 的"画命令列表"版。
// XAML 里写 <QCanvas Commands="Clear #313842; Rect 20,20,80,40,#00A8E8; ..."/>，
// 绘制层按分号切开逐条执行。命令都是数据，不需要跑脚本引擎，也就没有注入面。
//   Clear   #RRGGBB
//   Rect    x,y,w,h,#RRGGBB
//   RoundRect x,y,w,h,r,#RRGGBB
//   Ellipse x,y,w,h,#RRGGBB
//   Line    x1,y1,x2,y2,#RRGGBB[,thickness]
//   Text    x,y,文本,ptSize,#RRGGBB
// ===========================================================================
// ---------------------------------------------------------------------------
// 画布图元表：命令串 → 片段（命令名 + 参数 + 包围盒）。
//
// 绘制和"选中/删除"必须共用同一份切分语义，否则两边对"第几条"的理解一旦
// 不一致（比如空片段、空格分隔的 head），删除就会删错图元而画面看不出异常。
struct QcPiece
{
    std::wstring raw;                 // 原始片段文本（删除时按它重组命令串）
    std::vector<std::wstring> f;      // f[0]=命令名，f[1..]=参数
    RECT box = {0, 0, 0, 0};          // 控件客户区坐标
    bool selectable = false;          // 有确定包围盒、可以点选的图元
    bool locked = false;              // 命令名带 '!' 后缀：照常画，但不可选中（背景板用）
};

inline void qcSplit(const std::wstring& body, std::vector<QcPiece>& out)
{
    out.clear();
    size_t p = 0;
    while (p <= body.size())
    {
        const size_t semi = body.find(L';', p);
        QcPiece pc;
        pc.raw = body.substr(p, semi == std::wstring::npos ? std::wstring::npos : semi - p);
        p = (semi == std::wstring::npos) ? body.size() + 1 : semi + 1;
        if (pc.raw.empty())
            continue;

        std::wstring cur;
        for (wchar_t ch : pc.raw)
        {
            if (ch == L',')
            {
                pc.f.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(ch);
            }
        }
        pc.f.push_back(cur);
        for (auto& s : pc.f)
        {
            const size_t b = s.find_first_not_of(L" \t");
            const size_t e = s.find_last_not_of(L" \t");
            s = (b == std::wstring::npos) ? std::wstring() : s.substr(b, e - b + 1);
        }
        if (pc.f.empty() || pc.f[0].empty())
            continue;

        // 第一个逗号字段里同时藏着"命令名 + 第一个参数"（用空格分隔），拆开它。
        // 注意：没有空格说明是纯命令名（AlphaEnd / RotEnd 这类无参命令），必须原样往下分发，
        // 不能 continue —— 否则配对弹出的命令被整条跳过，图层/变换状态再也关不掉。
        if (pc.f[0].find_first_of(L" \t") != std::wstring::npos)
        {
            const std::wstring head = pc.f[0];
            const size_t sp = head.find_first_of(L" \t");
            const std::wstring rest = head.substr(sp + 1);
            pc.f.erase(pc.f.begin());
            if (!rest.empty())
                pc.f.insert(pc.f.begin(), rest);
            pc.f.insert(pc.f.begin(), head.substr(0, sp));
        }
        // 命令名末尾带 '!' = 锁定图元（底板/装饰线这类不该被点掉的东西）。
        // 去掉标记后照常绘制，只是不参与命中测试与删除。
        if (!pc.f[0].empty() && pc.f[0].back() == L'!')
        {
            pc.f[0].pop_back();
            pc.locked = true;
        }
        out.push_back(pc);
    }
}

// 只给"有确定包围盒"的图元算 box：矩形系（x,y,w,h）、线段、文字、折线/多边形。
// Clear / Alpha / Rot / AlphaEnd / RotEnd 这类是状态命令，不参与选中与删除。
inline bool qcBoxOf(QcPiece& pc, const RECT& rc)
{
    if (pc.locked)
        return false;
    const std::vector<std::wstring>& f = pc.f;
    auto n = [&](size_t i, int dflt) -> int {
        return (i < f.size() && !f[i].empty()) ? _wtoi(f[i].c_str()) : dflt;
    };
    const std::wstring& op = f[0];

    if (_wcsicmp(op.c_str(), L"Rect") == 0 || _wcsicmp(op.c_str(), L"RoundRect") == 0 ||
        _wcsicmp(op.c_str(), L"Ellipse") == 0 || _wcsicmp(op.c_str(), L"Grad") == 0 ||
        _wcsicmp(op.c_str(), L"GradRR") == 0 || _wcsicmp(op.c_str(), L"Shadow") == 0)
    {
        const int x = rc.left + n(1, 0);
        const int y = rc.top + n(2, 0);
        const int w = n(3, 0);
        const int h = n(4, 0);
        if (w <= 0 || h <= 0)
            return false;
        pc.box = RECT{x, y, x + w, y + h};
        pc.selectable = true;
        return true;
    }
    if (_wcsicmp(op.c_str(), L"Line") == 0)
    {
        const int ax = rc.left + n(1, 0);
        const int ay = rc.top + n(2, 0);
        const int bx = rc.left + n(3, 0);
        const int by = rc.top + n(4, 0);
        const int th = n(6, 1);
        const int pad = th > 0 ? th : 1;
        pc.box = RECT{(ax < bx ? ax : bx) - pad, (ay < by ? ay : by) - pad,
                      (ax > bx ? ax : bx) + pad, (ay > by ? ay : by) + pad};
        pc.selectable = true;
        return true;
    }
    if (_wcsicmp(op.c_str(), L"Text") == 0)
    {
        const int x = rc.left + n(1, 0);
        const int y = rc.top + n(2, 0);
        const std::wstring text = f.size() > 3 ? f[3] : std::wstring();
        const int pt = n(4, 9);
        // 文字没有真实度量：按字号估宽（0.62 倍字号/字），只用于命中与选中框
        int est = static_cast<int>(text.size() * (pt > 0 ? pt : 9) * 0.62);
        if (est < 12) est = 12;
        pc.box = RECT{x, y, x + est + 4, y + (pt > 0 ? pt * 2 : 18)};
        pc.selectable = true;
        return true;
    }
    if (_wcsicmp(op.c_str(), L"Path") == 0 || _wcsicmp(op.c_str(), L"Fill") == 0)
    {
        size_t ci = 0;
        for (size_t i = 1; i < f.size(); ++i)
        {
            if (!f[i].empty() && f[i][0] == L'#') { ci = i; break; }
        }
        if (ci < 3)
            return false;
        RECT b = {0, 0, 0, 0};
        int cnt = 0;
        for (size_t i = 1; i + 1 < ci; i += 2)
        {
            if (f[i].empty() || f[i + 1].empty())
                break;
            const int x = rc.left + _wtoi(f[i].c_str());
            const int y = rc.top + _wtoi(f[i + 1].c_str());
            if (cnt == 0) { b = RECT{x, y, x, y}; }
            else
            {
                if (x < b.left) b.left = x;
                if (y < b.top) b.top = y;
                if (x > b.right) b.right = x;
                if (y > b.bottom) b.bottom = y;
            }
            ++cnt;
        }
        if (cnt < 2)
            return false;
        pc.box = b;
        pc.selectable = true;
        return true;
    }
    return false;
}

inline std::vector<QcPiece> qcParse(const std::wstring& commands, const RECT& rc)
{
    std::vector<QcPiece> out;
    qcSplit(commands, out);
    for (auto& pc : out)
        qcBoxOf(pc, rc);
    return out;
}

// 选中框 + 四角控制柄 + 框选橡皮筋。画在画布自己的客户区里（画布是单个 HWND，
// 没有独立的 Adorner 窗口可用）。
inline void drawQcSelection(XamlCanvas& cv, const std::vector<QcPiece>& items,
                            const std::vector<int>* sel, const RECT* band)
{
    const COLORREF kSel = RGB(0, 132, 232);
    const COLORREF kHandle = RGB(240, 246, 255);
    if (sel != nullptr)
    {
        for (int k : *sel)
        {
            if (k < 0 || k >= static_cast<int>(items.size()) || !items[k].selectable)
                continue;
            RECT b = items[k].box;
            ::InflateRect(&b, 2, 2);
            cv.strokeRoundRect(b, kSel, 4, 1);
            const int s = 6;
            RECT hs[4] = {
                {b.left - s / 2, b.top - s / 2, b.left + s / 2, b.top + s / 2},
                {b.right - s / 2, b.top - s / 2, b.right + s / 2, b.top + s / 2},
                {b.left - s / 2, b.bottom - s / 2, b.left + s / 2, b.bottom + s / 2},
                {b.right - s / 2, b.bottom - s / 2, b.right + s / 2, b.bottom + s / 2}};
            for (const RECT& h : hs)
            {
                cv.fillRect(h, kHandle);
                cv.strokeRect(h, kSel, 1);
            }
        }
    }
    if (band != nullptr && band->right - band->left > 1 && band->bottom - band->top > 1)
        cv.strokeRect(*band, kSel, 1);
}

inline void drawQCanvas(XamlCanvas& cv, const RECT& rc, const std::wstring& commands,
                        const Palette& pal)
{
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    cv.fillRect(rc, pal.back);
    std::vector<QcPiece> pieces;
    qcSplit(commands, pieces);
    for (const QcPiece& pc : pieces)
    {
        std::vector<std::wstring> f = pc.f;

        auto num = [&](size_t i, int dflt) -> int {
            return (i < f.size() && !f[i].empty()) ? _wtoi(f[i].c_str()) : dflt;
        };
        auto col = [&](size_t i) -> COLORREF {
            COLORREF c = pal.fore;
            if (i < f.size() && !f[i].empty())
            {
                std::wstring hex = f[i];
                if (!hex.empty() && hex[0] == L'#')
                    hex = hex.substr(1);
                // 颜色串是十六进制（#RRGGBB），_wtoi 是按十进制读的，必须自己按 16 进制转
                if (hex.size() >= 6)
                {
                    auto hx = [](wchar_t ch) -> int {
                        if (ch >= L'0' && ch <= L'9') return static_cast<int>(ch - L'0');
                        if (ch >= L'a' && ch <= L'f') return static_cast<int>(ch - L'a' + 10);
                        if (ch >= L'A' && ch <= L'F') return static_cast<int>(ch - L'A' + 10);
                        return 0;
                    };
                    auto pair = [&](size_t i) -> int {
                        return hx(hex[i]) * 16 + hx(hex[i + 1]);
                    };
                    c = RGB(pair(0), pair(2), pair(4));
                }
            }
            return c;
        };

        const std::wstring op = f[0];
        if (_wcsicmp(op.c_str(), L"Clear") == 0)
        {
            cv.fillRect(rc, col(1));
            continue;
        }
        if (_wcsicmp(op.c_str(), L"Rect") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            cv.fillRect(r, col(5));
            continue;
        }
        if (_wcsicmp(op.c_str(), L"RoundRect") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            const COLORREF face = col(6);
            cv.fillRoundRect(r, face, face, num(5, 4));
            continue;
        }
        if (_wcsicmp(op.c_str(), L"Ellipse") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            const COLORREF face = col(5);
            cv.fillEllipse(r, face, face);
            continue;
        }
        if (_wcsicmp(op.c_str(), L"Line") == 0)
        {
            POINT a = { rc.left + num(1, 0), rc.top + num(2, 0) };
            POINT b = { rc.left + num(3, 0), rc.top + num(4, 0) };
            cv.strokeLine(a, b, col(5), num(6, 1) > 0 ? num(6, 1) : 1);
            continue;
        }
        if (_wcsicmp(op.c_str(), L"Text") == 0)
        {
            const int x = rc.left + num(1, 0);
            const int y = rc.top + num(2, 0);
            const std::wstring text = f.size() > 3 ? f[3] : std::wstring();
            RECT tr = { x, y, rc.right, y + 22 };
            cv.text(tr, text, col(5), DT_LEFT | DT_TOP | DT_SINGLELINE,
                    num(4, pal.ptSize), pal.face);
            continue;
        }

        // ---------------- P0 原语：渐变 / 路径 / 阴影 / 图层透明度 / 旋转 ----------------
        // Grad   x,y,w,h,#c1,#c2[,angle]        线性渐变矩形（angle 0=左→右，90=上→下）
        // GradRR x,y,w,h,r,#c1,#c2[,angle]      圆角 + 渐变
        if (_wcsicmp(op.c_str(), L"Grad") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            cv.fillRectGradient(r, col(5), col(6), num(7, 0));
            continue;
        }
        if (_wcsicmp(op.c_str(), L"GradRR") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            cv.fillRoundRectGradient(r, col(6), col(7), num(8, 0), num(5, 6));
            continue;
        }
        // Path x1,y1,...,xn,yn,#color[,thickness]  开放折线描边
        // Fill x1,y1,...,xn,yn,#color             多边形填充（>=3 点）
        if (_wcsicmp(op.c_str(), L"Path") == 0 || _wcsicmp(op.c_str(), L"Fill") == 0)
        {
            size_t ci = 0;
            for (size_t i = 1; i < f.size(); ++i)
            {
                if (!f[i].empty() && f[i][0] == L'#') { ci = i; break; }
            }
            if (ci == 0 || ci < 3)
                continue;
            std::vector<POINT> pts;
            for (size_t i = 1; i + 1 < ci; i += 2)
            {
                if (f[i].empty() || f[i + 1].empty()) break;
                POINT pt = { rc.left + _wtoi(f[i].c_str()), rc.top + _wtoi(f[i + 1].c_str()) };
                pts.push_back(pt);
            }
            const COLORREF c = col(ci);
            if (_wcsicmp(op.c_str(), L"Fill") == 0)
            {
                if (pts.size() >= 3) cv.fillPath(pts, c);
            }
            else
            {
                const int th = (ci + 1 < f.size()) ? num(ci + 1, 1) : 1;
                cv.strokePath(pts, c, th > 0 ? th : 1, false);
            }
            continue;
        }
        // Shadow x,y,w,h,r,#color[,alpha[,dx,dy[,blur]]]
        if (_wcsicmp(op.c_str(), L"Shadow") == 0)
        {
            RECT r = { rc.left + num(1, 0), rc.top + num(2, 0),
                       rc.left + num(1, 0) + num(3, 0), rc.top + num(2, 0) + num(4, 0) };
            const COLORREF c = col(6);
            const int alpha = (f.size() > 7) ? num(7, 120) : 120;
            const int dx = (f.size() > 8) ? num(8, 0) : 0;
            const int dy = (f.size() > 9) ? num(9, 4) : 4;
            const int blur = (f.size() > 10) ? num(10, 8) : 8;
            cv.fillSoftShadow(r, num(5, 6), c, alpha, blur, dx, dy);
            continue;
        }
        // Alpha 0..255 / AlphaEnd：成对的组透明度图层（形状与文字统一混合）
        if (_wcsicmp(op.c_str(), L"Alpha") == 0)
        {
            cv.pushOpacity(num(1, 255));
            continue;
        }
        if (_wcsicmp(op.c_str(), L"AlphaEnd") == 0)
        {
            cv.popOpacity();
            continue;
        }
        // Rot deg[,cx,cy] / RotEnd：绕画布内某点旋转后续绘制
        if (_wcsicmp(op.c_str(), L"Rot") == 0)
        {
            POINT center = { rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2 };
            if (f.size() > 3 && !f[2].empty() && !f[3].empty())
            {
                center.x = rc.left + _wtoi(f[2].c_str());
                center.y = rc.top + _wtoi(f[3].c_str());
            }
            cv.pushRotate(num(1, 0), center);
            continue;
        }
        if (_wcsicmp(op.c_str(), L"RotEnd") == 0)
        {
            cv.popTransform();
            continue;
        }
    }
}
}  // namespace pymfcskin
