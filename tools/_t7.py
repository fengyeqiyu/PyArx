import io
h='G:/PyArx/PyMfcXaml/Table.h'
s=io.open(h,encoding='utf-8-sig').read()
s=s.replace('''    std::vector<int> m_view;                          // 过滤后的可见行（存数据行号）''',
'''    std::vector<int> m_view;                          // 过滤后的可见行（存数据行号）
    int m_scrollTop = 0;                              // 纵向滚动量（像素）''')
s=s.replace('''    void ApplySort(int col, bool desc);''',
'''    void ApplySort(int col, bool desc);
    int  MaxScroll() const { return (std::max)(0, ViewRows() * m_rowH - (Rect().bottom - m_headerH)); }
    void ScrollBy(int dy) { m_scrollTop += dy; const int mx = MaxScroll();
                            if (m_scrollTop < 0) m_scrollTop = 0; if (m_scrollTop > mx) m_scrollTop = mx; Invalidate(); }''')
s=s.replace('#include <vector>','#include <vector>\n#include <algorithm>')
io.open(h,'w',encoding='utf-8-sig',newline='').write(s)

c='G:/PyArx/PyMfcXaml/Table.cpp'
t=io.open(c,encoding='utf-8-sig').read()
# 几何减去滚动
t=t.replace('''    rc.top = m_headerH + pos * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

RECT TableControl::Cell''','''    rc.top = m_headerH + pos * m_rowH - m_scrollTop;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

RECT TableControl::Cell''')
t=t.replace('''    rc.top = m_headerH + ViewPos(row) * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}''','''    rc.top = m_headerH + ViewPos(row) * m_rowH - m_scrollTop;
    rc.bottom = rc.top + m_rowH;
    return rc;
}''')
t=t.replace('''    if (y < m_headerH) return false;
    const int pos = (y - m_headerH) / m_rowH;''','''    if (y < m_headerH) return false;
    const int pos = (y - m_headerH + m_scrollTop) / m_rowH;''')
# 滚轮
t=t.replace('''    case WM_MOUSEWHEEL:
    {
        ::SendMessageW(::GetParent(hwnd), msg, wp, lp);
        return 0;
    }''','''    case WM_MOUSEWHEEL:
    {
        // 自己滚，不再转发父窗口（转发会让整窗跟着重画，是闪烁来源之一）
        const int delta = GET_WHEEL_DELTA_WPARAM(wp);
        self->ScrollBy(-delta / 120 * self->m_rowH * 3);
        return 0;
    }''')
# 双缓冲绘制
t=t.replace('''    PAINTSTRUCT ps = {};
    HDC dc = ::BeginPaint(m_hwnd, &ps);
    const RECT rc = Rect();
    ::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_WINDOW));''','''    PAINTSTRUCT ps = {};
    HDC dc = ::BeginPaint(m_hwnd, &ps);
    const RECT rc = Rect();

    // 双缓冲：整块画进内存 DC，最后一次 BitBlt，滚动时不闪
    HDC mem = ::CreateCompatibleDC(dc);
    HBITMAP bmp = ::CreateCompatibleBitmap(dc, rc.right, rc.bottom);
    HGDIOBJ oldBmp = ::SelectObject(mem, bmp);
    ::FillRect(mem, &rc, ::GetSysColorBrush(COLOR_WINDOW));
    HDC dcSave = dc;
    dc = mem;''')
t=t.replace('''    ::SelectObject(dc, oldFont);
    ::DeleteObject(font);
    ::EndPaint(m_hwnd, &ps);
    return 0;''','''    ::SelectObject(dc, oldFont);
    ::DeleteObject(font);

    ::BitBlt(dcSave, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    ::SelectObject(mem, oldBmp);
    ::DeleteObject(bmp);
    ::DeleteDC(mem);
    ::EndPaint(m_hwnd, &ps);
    return 0;''')
io.open(c,'w',encoding='utf-8-sig',newline='').write(t)
print('双缓冲 + 滚动已加入')
