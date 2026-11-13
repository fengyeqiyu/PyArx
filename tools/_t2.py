import io
p='G:/PyArx/PyMfcXaml/Table.h'
s=io.open(p,encoding='utf-8-sig').read()
add='''

// =====================================================================
// TableControl —— 独立控件套装：注册自己的窗口类，自带数据(rows/cols)、
// 自带几何(rect/cell)、自带绘制与命中，不再依赖宿主内嵌 SysListView32。
//   Register(hInst)  注册窗口类 "PyMfcTable"
//   Create(parent,x,y,w,h)  创建实例，返回 HWND
//   AppendRow / SetCell / Cell / CellRect / RowRect / Rect
//   Select(row) / Selected()
//   SetCallback(fn, ctx)   命中/选中/按键回调（fn(void*,int row,int col,int what)）
// what: 1=单击 2=双击 3=选中行变化 4=F2 5=Delete
// =====================================================================
typedef void (*TableEventFn)(void* ctx, int row, int col, int what);

class TableControl
{
public:
    static bool Register(HINSTANCE hInst);
    static HWND Create(HWND parent, int x, int y, int w, int h, int id);
    static TableControl* From(HWND hwnd);

    void AppendRow(const std::vector<std::wstring>& cells);
    void Clear();
    void SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles);
    void SetCell(int row, int col, const std::wstring& text);

    int Rows() const { return static_cast<int>(m_rows.size()); }
    int Cols() const { return static_cast<int>(m_colW.size()); }
    RECT Rect() const;
    RECT RowRect(int row) const;
    RECT Cell(int row, int col) const;
    bool Hit(int x, int y, int& row, int& col) const;

    int Selected() const { return m_sel; }
    void Select(int row);

    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }
    void SetTextStyle(const std::wstring& face, int pt, COLORREF fg, COLORREF line,
                      COLORREF selBg, COLORREF hdrBg);
    void Invalidate();

private:
    HWND m_hwnd = nullptr;
    int m_id = 0;
    std::vector<std::vector<std::wstring>> m_rows;   // rows
    std::vector<int> m_colW;                         // cols
    std::vector<std::wstring> m_colTitle;
    int m_sel = -1;
    int m_headerH = 24, m_rowH = 21;
    std::wstring m_face = L"Microsoft YaHei UI";
    int m_pt = 9;
    COLORREF m_fg = RGB(0x1F,0x29,0x37), m_line = RGB(0xE4,0xE7,0xEC),
             m_selBg = RGB(0x1F,0x6F,0xEB), m_hdrBg = RGB(0xF7,0xF8,0xFA);
    TableEventFn m_fn = nullptr;
    void* m_ctx = nullptr;

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT OnPaint();
    void Fire(int row, int col, int what);
};
'''
if 'class TableControl' not in s:
    s=s.rstrip()+'\n'+add
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)

p2='G:/PyArx/PyMfcXaml/Table.cpp'
c=io.open(p2,encoding='utf-8-sig').read()
if 'TableControl::Register' not in c:
    c=c.rstrip()+'''


// =============================== TableControl ===============================
namespace { const wchar_t* kTableCls = L"PyMfcTable"; const wchar_t* kTableProp = L"PyMfcTableSelf"; }

bool TableControl::Register(HINSTANCE hInst)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &TableControl::WndProc;
    wc.hInstance = hInst;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kTableCls;
    wc.hbrBackground = nullptr;
    if (::RegisterClassExW(&wc) != 0) return true;
    return ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

HWND TableControl::Create(HWND parent, int x, int y, int w, int h, int id)
{
    TableControl::Register(reinterpret_cast<HINSTANCE>(::GetModuleHandleW(nullptr)));
    TableControl* self = new TableControl();
    HWND hw = ::CreateWindowExW(0, kTableCls, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                ::GetModuleHandleW(nullptr), nullptr);
    if (hw == nullptr) { delete self; return nullptr; }
    self->m_hwnd = hw;
    self->m_id = id;
    ::SetPropW(hw, kTableProp, reinterpret_cast<HANDLE>(self));
    return hw;
}

TableControl* TableControl::From(HWND hwnd)
{
    return reinterpret_cast<TableControl*>(::GetPropW(hwnd, kTableProp));
}

void TableControl::AppendRow(const std::vector<std::wstring>& cells)
{ m_rows.push_back(cells); Invalidate(); }

void TableControl::Clear() { m_rows.clear(); m_sel = -1; Invalidate(); }

void TableControl::SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles)
{ m_colW = widths; m_colTitle = titles; Invalidate(); }

void TableControl::SetCell(int row, int col, const std::wstring& text)
{
    if (row < 0 || row >= Rows() || col < 0 || col >= Cols()) return;
    if (static_cast<int>(m_rows[row].size()) <= col) m_rows[row].resize(col + 1);
    m_rows[row][col] = text;
    Invalidate();
}

RECT TableControl::Rect() const
{
    RECT rc = { 0,0,0,0 };
    if (m_hwnd && ::IsWindow(m_hwnd)) ::GetClientRect(m_hwnd, &rc);
    return rc;
}

RECT TableControl::RowRect(int row) const
{
    RECT rc = { 0,0,0,0 };
    if (row < 0 || row >= Rows()) return rc;
    rc.left = 0;
    rc.top = m_headerH + row * m_rowH;
    rc.right = Rect().right;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

RECT TableControl::Cell(int row, int col) const
{
    RECT rc = { 0,0,0,0 };
    if (row < 0 || row >= Rows() || col < 0 || col >= Cols()) return rc;
    int x = 0;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    rc.left = x;
    rc.right = x + m_colW[col];
    rc.top = m_headerH + row * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

bool TableControl::Hit(int x, int y, int& row, int& col) const
{
    row = -1; col = -1;
    if (y < m_headerH) return false;
    const int r = (y - m_headerH) / m_rowH;
    if (r < 0 || r >= Rows()) return false;
    int acc = 0, c = -1;
    for (int i = 0; i < Cols(); ++i)
    {
        if (x >= acc && x < acc + m_colW[i]) { c = i; break; }
        acc += m_colW[i];
    }
    if (c < 0) return false;
    row = r; col = c;
    return true;
}

void TableControl::Select(int row)
{
    if (Rows() <= 0) { m_sel = -1; return; }
    m_sel = row < 0 ? -1 : (row >= Rows() ? Rows() - 1 : row);
    Invalidate();
}

void TableControl::SetTextStyle(const std::wstring& face, int pt, COLORREF fg, COLORREF line,
                                COLORREF selBg, COLORREF hdrBg)
{
    m_face = face; m_pt = pt; m_fg = fg; m_line = line; m_selBg = selBg; m_hdrBg = hdrBg;
    Invalidate();
}

void TableControl::Invalidate()
{ if (m_hwnd && ::IsWindow(m_hwnd)) ::InvalidateRect(m_hwnd, nullptr, FALSE); }

void TableControl::Fire(int row, int col, int what)
{ if (m_fn) m_fn(m_ctx, row, col, what); }

LRESULT CALLBACK TableControl::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TableControl* self = TableControl::From(hwnd);
    if (self == nullptr) return ::DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        return self->OnPaint();
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        int row = -1, col = -1;
        if (self->Hit(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), row, col))
        {
            if (row != self->m_sel) { self->Select(row); self->Fire(row, col, 3); }
            self->Fire(row, col, msg == WM_LBUTTONDBLCLK ? 2 : 1);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_F2)      { self->Fire(self->m_sel, 0, 4); return 0; }
        if (wp == VK_DELETE)  { self->Fire(self->m_sel, 0, 5); return 0; }
        if (wp == VK_DOWN || wp == VK_UP)
        {
            const int next = self->m_sel < 0 ? 0 : self->m_sel + (wp == VK_DOWN ? 1 : -1);
            self->Select(next);
            self->Fire(self->m_sel, 0, 3);
            return 0;
        }
        break;
    case WM_MOUSEWHEEL:
    {
        ::SendMessageW(::GetParent(hwnd), msg, wp, lp);
        return 0;
    }
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT TableControl::OnPaint()
{
    PAINTSTRUCT ps = {};
    HDC dc = ::BeginPaint(m_hwnd, &ps);
    const RECT rc = Rect();
    ::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_WINDOW));

    const int logPx = ::MulDiv(m_pt, ::GetDeviceCaps(dc, LOGPIXELSY), 72);
    HFONT font = ::CreateFontW(-logPx, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, m_face.c_str());
    HGDIOBJ oldFont = ::SelectObject(dc, font);
    ::SetBkMode(dc, TRANSPARENT);

    // 表头
    RECT hdr = { rc.left, rc.top, rc.right, rc.top + m_headerH };
    HBRUSH hb = ::CreateSolidBrush(m_hdrBg);
    ::FillRect(dc, &hdr, hb);
    ::DeleteObject(hb);
    ::SetTextColor(dc, m_fg);
    int x = 0;
    for (int i = 0; i < Cols(); ++i)
    {
        RECT cellRc = { x + 6, hdr.top, x + m_colW[i] - 4, hdr.bottom };
        const std::wstring t = i < static_cast<int>(m_colTitle.size()) ? m_colTitle[i] : L"";
        ::DrawTextW(dc, t.c_str(), -1, &cellRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        x += m_colW[i];
    }

    // 行
    const int rows = Rows();
    for (int r = 0; r < rows; ++r)
    {
        RECT rr = RowRect(r);
        if (rr.top >= rc.bottom) break;
        if (r == m_sel)
        {
            HBRUSH sb = ::CreateSolidBrush(m_selBg);
            ::FillRect(dc, &rr, sb);
            ::DeleteObject(sb);
            ::SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
        }
        else ::SetTextColor(dc, m_fg);

        int cx = 0;
        for (int c = 0; c < Cols(); ++c)
        {
            RECT cellRc = { cx + 6, rr.top, cx + m_colW[c] - 4, rr.bottom };
            const std::wstring t = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : L"";
            ::DrawTextW(dc, t.c_str(), -1, &cellRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            cx += m_colW[c];
        }
        HBRUSH lb = ::CreateSolidBrush(m_line);
        RECT lineRc = { rc.left, rr.bottom - 1, rc.right, rr.bottom };
        ::FillRect(dc, &lineRc, lb);
        ::DeleteObject(lb);
    }

    ::SelectObject(dc, oldFont);
    ::DeleteObject(font);
    ::EndPaint(m_hwnd, &ps);
    return 0;
}
'''
io.open(p2,'w',encoding='utf-8-sig',newline='').write(c)
print('TableControl 已加入')
