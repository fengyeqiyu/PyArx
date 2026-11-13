import io
p2='G:/PyArx/PyMfcXaml/Table.cpp'
c=io.open(p2,encoding='utf-8-sig').read()
# 去掉重复的 Clear（原先已有）
c=c.replace('''// ------------------------- 过滤视图 / 排序 -------------------------
void TableControl::Clear()
{ m_rows.clear(); m_sel = -1; m_view.clear(); Invalidate(); }

void TableControl::RebuildView()''','''// ------------------------- 过滤视图 / 排序 -------------------------
void TableControl::RebuildView()''')
# 原 Clear 里补 m_view
c=c.replace('''void TableControl::Clear() { m_rows.clear(); m_sel = -1; Invalidate(); }''',
'''void TableControl::Clear() { m_rows.clear(); m_sel = -1; m_view.clear(); Invalidate(); }''')
# 头文件补充
c=c.replace('#include "Table.h"','#include "Table.h"\n#include <algorithm>\n#include <cmath>\n#include <cwctype>')
# 表头点击 + 双击菜单
c=c.replace('''    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        int row = -1, col = -1;''',
'''    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        // 表头区域：点漏斗图标 = 输入筛选；双击表头 = 弹出菜单
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int col = self->ColAtX(GET_X_LPARAM(lp));
            if (col >= 0)
            {
                int colRight = 0;
                for (int i = 0; i <= col; ++i) colRight += self->m_colW[i];
                const bool onFunnel = GET_X_LPARAM(lp) >= colRight - 20;
                if (msg == WM_LBUTTONDBLCLK)      self->RunHeaderMenu(col);
                else if (onFunnel)                self->RunHeaderFilter(col);
            }
            return 0;
        }
        int row = -1, col = -1;''')
# 表头绘制加漏斗
c=c.replace('''        ::DrawTextW(dc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        x += m_colW[i];''',
'''        cellRc.right -= 18;   // 给漏斗图标留位
        ::DrawTextW(dc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 漏斗图标：有筛选词时用主题蓝，否则浅灰
        RECT ic = { x + m_colW[i] - 18, hdr.top, x + m_colW[i] - 4, hdr.bottom };
        const std::wstring f = FilterOf(i);
        ::SetTextColor(dc, f.empty() ? RGB(0x9A,0xA4,0xB2) : RGB(0x1F,0x6F,0xEB));
        const std::wstring glyph = f.empty() ? L"\\u25BD" : L"\\u25BC";
        ::DrawTextW(dc, glyph.c_str(), -1, &ic, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        ::SetTextColor(dc, m_fg);
        x += m_colW[i];''')
# 表头行循环：加上排序指示
c=c.replace('''    // 行
    const int rows = Rows();
    for (int r = 0; r < rows; ++r)
    {
        RECT rr = RowRect(r);
        if (rr.top >= rc.bottom) break;''',
'''    // 行（按过滤视图画）
    const int rows = ViewRows();
    for (int pos = 0; pos < rows; ++pos)
    {
        const int r = m_view[pos];
        RECT rr = RowRect(r);
        if (rr.top >= rc.bottom) break;''')
# 追加 RunHeaderFilter / RunHeaderMenu
c=c.replace('''void TableControl::ApplySort(int col, bool desc) { SortBy(col, desc); }''',
'''void TableControl::ApplySort(int col, bool desc) { SortBy(col, desc); }

// 点漏斗：在表头格上临时开一个 EDIT，模态收字，回车应用 / Esc 取消
void TableControl::RunHeaderFilter(int col)
{
    int x = 0;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    RECT er = { x, 0, x + m_colW[col], m_headerH };
    er.left += 2; er.right -= 20;

    m_hFilterEdit = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                      WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                      er.left, er.top, er.right - er.left, er.bottom - er.top,
                                      m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(9001)),
                                      ::GetModuleHandleW(nullptr), nullptr);
    if (m_hFilterEdit == nullptr) return;

    ::SendMessageW(m_hFilterEdit, WM_SETTEXT, 0,
                   reinterpret_cast<LPARAM>(FilterOf(col).c_str()));
    ::SendMessageW(m_hFilterEdit, EM_SETSEL, 0, -1);
    ::SetFocus(m_hFilterEdit);

    MSG m;
    bool ok = false;
    while (::GetMessageW(&m, m_hFilterEdit, 0, 0))
    {
        if (m.message == WM_KEYDOWN && m.wParam == VK_RETURN) { ok = true; break; }
        if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) { break; }
        ::TranslateMessage(&m);
        ::DispatchMessageW(&m);
    }

    if (ok)
    {
        std::wstring text(256, L'\\0');
        const int n = static_cast<int>(::SendMessageW(m_hFilterEdit, WM_GETTEXT,
                                                      static_cast<WPARAM>(text.size()),
                                                      reinterpret_cast<LPARAM>(text.data())));
        if (n > 0) text.resize(n);
        SetFilter(col, text);
    }
    ::DestroyWindow(m_hFilterEdit);
    m_hFilterEdit = nullptr;
    ::SetFocus(m_hwnd);
    Invalidate();
}

// 双击表头：弹出菜单（升序 / 降序 / 清本列筛选 / 清全部筛选）
void TableControl::RunHeaderMenu(int col)
{
    HMENU menu = ::CreatePopupMenu();
    ::AppendMenuW(menu, MF_STRING, 1, L"升序排序");
    ::AppendMenuW(menu, MF_STRING, 2, L"降序排序");
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, 3, L"清除该列筛选");
    ::AppendMenuW(menu, MF_STRING, 4, L"清除全部筛选");

    POINT pt = { 0, 0 };
    ::GetCursorPos(&pt);
    const int cmd = static_cast<int>(::TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN,
                                                       pt.x, pt.y, 0, m_hwnd, nullptr));
    ::DestroyMenu(menu);

    if (cmd == 1) SortBy(col, false);
    else if (cmd == 2) SortBy(col, true);
    else if (cmd == 3) SetFilter(col, L"");
    else if (cmd == 4) ClearFilters();
}''')
io.open(p2,'w',encoding='utf-8-sig',newline='').write(c)
print('表头过滤器与菜单已加入')
