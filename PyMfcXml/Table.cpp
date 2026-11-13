#include "stdafx.h"
#include "Table.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <set>

namespace
{
    // 点表头漏斗：用自定义消息延后弹筛选框，避免在鼠标消息处理里跑模态循环
    const UINT kMsgHeaderFilter = WM_APP + 41;

    bool isEmpty(const RECT& r)
    {
        return r.right <= r.left || r.bottom <= r.top;
    }

    void setClipboardText(HWND owner, const std::wstring& text)
    {
        if (!::OpenClipboard(owner)) return;
        ::EmptyClipboard();
        const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        if (HGLOBAL mem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes))
        {
            if (void* p = ::GlobalLock(mem))
            {
                ::memcpy(p, text.c_str(), bytes);
                ::GlobalUnlock(mem);
                ::SetClipboardData(CF_UNICODETEXT, mem);
            }
        }
        ::CloseClipboard();
    }

    std::wstring getClipboardText(HWND owner)
    {
        std::wstring out;
        if (!::OpenClipboard(owner)) return out;
        if (HANDLE h = ::GetClipboardData(CF_UNICODETEXT))
        {
            if (const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(h)))
            {
                out = p;
                ::GlobalUnlock(h);
            }
        }
        ::CloseClipboard();
        return out;
    }
}

int Table::rows() const
{
    if (!valid()) return 0;
    return static_cast<int>(::SendMessageW(m_hwnd, LVM_GETITEMCOUNT, 0, 0));
}

int Table::cols() const
{
    if (!valid()) return 0;
    HWND hh = reinterpret_cast<HWND>(::SendMessageW(m_hwnd, LVM_GETHEADER, 0, 0));
    if (hh == nullptr) return 0;
    return static_cast<int>(::SendMessageW(hh, HDM_GETITEMCOUNT, 0, 0));
}

std::vector<int> Table::colWidths() const
{
    std::vector<int> ws;
    const int n = cols();
    ws.reserve(n);
    for (int i = 0; i < n; ++i)
        ws.push_back(static_cast<int>(::SendMessageW(m_hwnd, LVM_GETCOLUMNWIDTH,
                                                     static_cast<WPARAM>(i), 0)));
    return ws;
}

RECT Table::rect() const
{
    RECT rc = { 0, 0, 0, 0 };
    if (valid()) ::GetClientRect(m_hwnd, &rc);
    return rc;
}

RECT Table::rowRect(int row) const
{
    RECT rc = { 0, 0, 0, 0 };
    if (!valid() || row < 0 || row >= rows()) return rc;
    if (::SendMessageW(m_hwnd, LVM_GETITEMRECT, static_cast<WPARAM>(row),
                       reinterpret_cast<LPARAM>(&rc)) == FALSE)
    {
        RECT empty = { 0, 0, 0, 0 };
        return empty;
    }
    return rc;
}

int Table::headerHeight() const
{
    const RECT r0 = rowRect(0);
    return isEmpty(r0) ? 0 : r0.top;
}

int Table::rowHeight() const
{
    const RECT r0 = rowRect(0);
    return isEmpty(r0) ? 0 : (r0.bottom - r0.top);
}

RECT Table::cell(int row, int col) const
{
    RECT rc = { 0, 0, 0, 0 };
    if (!valid() || row < 0 || col < 0 || col >= cols()) return rc;
    const RECT rr = rowRect(row);
    if (isEmpty(rr)) return rc;

    // LVM_GETSUBITEMRECT 在本机 SysListView32 上水平方向恒返回 0 宽，
    // 所以按"行矩形 + 累加列宽"算左右边界；纵向直接用行矩形。
    int x = 0, w = 0;
    for (int i = 0; i <= col; ++i)
    {
        const int cw = static_cast<int>(::SendMessageW(m_hwnd, LVM_GETCOLUMNWIDTH,
                                                       static_cast<WPARAM>(i), 0));
        if (cw <= 0) { RECT empty = { 0, 0, 0, 0 }; return empty; }
        if (i < col) x += cw; else w = cw;
    }
    POINT org = { 0, 0 };
    ::SendMessageW(m_hwnd, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
    rc.left = x - org.x;
    rc.right = rc.left + w;
    rc.top = rr.top - org.y;
    rc.bottom = rr.bottom - org.y;
    return rc;
}

int Table::rowAt(int y) const
{
    if (!valid()) return -1;
    const int n = rows();
    if (n <= 0) return -1;
    const int hh = headerHeight();
    const int rh = rowHeight();
    if (rh <= 0) return -1;
    POINT org = { 0, 0 };
    ::SendMessageW(m_hwnd, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
    const int yy = y + org.y;
    if (yy < hh) return -1;
    const int idx = (yy - hh) / rh;
    if (idx < 0 || idx >= n) return -1;
    return idx;
}

bool Table::cellAt(int x, int y, int& row, int& col) const
{
    row = -1;
    col = -1;
    if (!valid()) return false;
    LVHITTESTINFO hi = {};
    hi.pt.x = x;
    hi.pt.y = y;
    ::SendMessageW(m_hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
    if (hi.iItem < 0 || hi.iItem >= rows())
    {
        // 交回给 rowAt 兜底（表头/空白行被 hittest 吃掉时也不会差一行）
        const int r = rowAt(y);
        if (r < 0) return false;
        row = r;
        const std::vector<int> ws = colWidths();
        int acc = 0;
        for (int i = 0; i < static_cast<int>(ws.size()); ++i)
        {
            if (x >= acc && x < acc + ws[i]) { col = i; break; }
            acc += ws[i];
        }
        if (col < 0) col = 0;
        return true;
    }
    row = hi.iItem;
    col = hi.iSubItem >= 0 ? hi.iSubItem : 0;
    return true;
}

int Table::selectedRow() const
{
    if (!valid()) return -1;
    const int r = static_cast<int>(::SendMessageW(m_hwnd, LVM_GETNEXTITEM,
                                                  static_cast<WPARAM>(-1), LVNI_SELECTED));
    return r;
}

void Table::selectRow(int row)
{
    if (!valid()) return;
    const int n = rows();
    if (n <= 0) return;
    int v = row < 0 ? -1 : (row >= n ? n - 1 : row);
    if (v < 0) return;
    LVITEMW lv = {};
    lv.mask = LVIF_STATE;
    lv.state = LVIS_SELECTED | LVIS_FOCUSED;
    lv.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ::SendMessageW(m_hwnd, LVM_SETITEMSTATE, static_cast<WPARAM>(v),
                   reinterpret_cast<LPARAM>(&lv));
}

void Table::ensureVisible(int row)
{
    if (!valid()) return;
    ::SendMessageW(m_hwnd, LVM_ENSUREVISIBLE, static_cast<WPARAM>(row), FALSE);
}


// =============================== TableControl ===============================
static const int kSbH = 10;      // 底部横向滚动条高度（与右侧纵条同宽）

namespace { const wchar_t* kTableCls = L"PyMfcTable"; const wchar_t* kTableProp = L"PyMfcTableSelf";
            const wchar_t* kFilterCls = L"PyMfcFilterPopup"; }

// 表头漏斗的下拉面板：照 Excel —— 顶部搜索框，下面复选列表（全选 / 空白 / 各值），底部确定取消
struct FilterPopupData
{
    TableControl* owner = nullptr;
    int col = -1;
    std::vector<std::wstring> vals;   // 该列去重后的值（含空串表示空白）
    std::wstring query;               // 搜索框里的过滤词
    std::vector<char> checked;        // 与 vals 一一对应的勾选态
    std::vector<int> idxs;            // 搜索过滤后的可见索引
    int scroll = 0;
    RECT rc = { 0, 0, 0, 0 };         // 面板在表格客户区里的矩形（直接画在表格身上）
};

namespace
{
    const int kFpRowH = 22;
    const int kFpListTop = 40;
    const int kFpBtnH = 26;
    const int kFpBtnW = 64;

    std::vector<std::wstring> DistinctValues(TableControl* t, int col)
    {
        std::set<std::wstring> seen;
        std::vector<std::wstring> out;
        const int rows = t->Rows();
        for (int r = 0; r < rows && static_cast<int>(out.size()) < 5000; ++r)
        {
            const std::vector<std::wstring> cells = t->RowAt(r);
            std::wstring v = (col < static_cast<int>(cells.size())) ? cells[col] : L"";
            // 去掉首尾空格再比较，避免 "GRID " 与 "GRID" 分成两项
            size_t a = v.find_first_not_of(L" \t\r\n");
            size_t b = v.find_last_not_of(L" \t\r\n");
            v = (a == std::wstring::npos) ? L"" : v.substr(a, b - a + 1);
            if (seen.insert(v).second) out.push_back(v);
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    bool ContainsNoCase(const std::wstring& hay, const std::wstring& needle)
    {
        if (needle.empty()) return true;
        std::wstring a = hay, b = needle;
        for (wchar_t& ch : a) ch = static_cast<wchar_t>(::towlower(ch));
        for (wchar_t& ch : b) ch = static_cast<wchar_t>(::towlower(ch));
        return a.find(b) != std::wstring::npos;
    }

    void RefreshFiltered(FilterPopupData* d)
    {
        d->idxs.clear();
        for (int i = 0; i < static_cast<int>(d->vals.size()); ++i)
        {
            const std::wstring& v = d->vals[i];
            if (v.empty()) { if (d->query.empty()) d->idxs.push_back(i); }
            else if (ContainsNoCase(v, d->query)) d->idxs.push_back(i);
        }
    }

    // 画一个 12×12 的复选框
    void DrawCheck(HDC dc, int x, int y, bool on)    {
        RECT box = { x, y, x + 12, y + 12 };
        HBRUSH b = ::CreateSolidBrush(on ? RGB(0x1F, 0x6F, 0xEB) : RGB(0xFF, 0xFF, 0xFF));
        ::FillRect(dc, &box, b);
        ::DeleteObject(b);
        // 用 FrameRect 描边：Rectangle 会先用 DC 当前画刷（默认白）把内部再填一遍，
        // 把上面刚填的蓝底盖掉，就只剩蓝框白底、勾也看不见。
        HBRUSH pb = ::CreateSolidBrush(on ? RGB(0x1F, 0x6F, 0xEB) : RGB(0xB4, 0xBC, 0xC8));
        ::FrameRect(dc, &box, pb);
        ::DeleteObject(pb);
        if (on)
        {
            HPEN cp = ::CreatePen(PS_SOLID, 1, RGB(0xFF, 0xFF, 0xFF));
            HGDIOBJ oc = ::SelectObject(dc, cp);
            ::MoveToEx(dc, x + 3, y + 6, nullptr); ::LineTo(dc, x + 5, y + 9);
            ::MoveToEx(dc, x + 5, y + 9, nullptr); ::LineTo(dc, x + 10, y + 3);
            ::SelectObject(dc, oc); ::DeleteObject(cp);
        }
    }

}

// 面板整体画在表格自己的 DC 上（不再开独立窗口）：先画底，再按 d->rc 内的相对坐标画各行与按钮
static void DrawFilterPanel(FilterPopupData* d, HDC dc)
{
    const RECT& rc = d->rc;
    HBRUSH bg = ::CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
    ::FillRect(dc, &rc, bg);
    ::DeleteObject(bg);
    HPEN border = ::CreatePen(PS_SOLID, 1, RGB(0x9A, 0xA6, 0xB6));
    HGDIOBJ oldPen0 = ::SelectObject(dc, border);
    ::Rectangle(dc, rc.left, rc.top, rc.right - 1, rc.bottom - 1);
    ::SelectObject(dc, oldPen0);
    ::DeleteObject(border);

    // 搜索框
    RECT sr = { rc.left + 8, rc.top + 8, rc.right - 8, rc.top + 32 };
    HBRUSH wb = ::CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
    ::FillRect(dc, &sr, wb);
    ::DeleteObject(wb);
    HPEN sp = ::CreatePen(PS_SOLID, 1, RGB(0xC9, 0xCE, 0xD6));
    ::SelectObject(dc, sp);
    ::Rectangle(dc, sr.left, sr.top, sr.right, sr.bottom);
    ::DeleteObject(sp);

    HFONT font = ::CreateFontW(-MulDiv(9, ::GetDeviceCaps(dc, LOGPIXELSY), 72), 0, 0, 0,
                               FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, L"Microsoft YaHei UI");
    HGDIOBJ oldFont = ::SelectObject(dc, font);
    ::SetBkMode(dc, TRANSPARENT);
    ::SetTextColor(dc, RGB(0x1F, 0x29, 0x37));
    RECT qr = { sr.left + 4, sr.top + 3, sr.right - 4, sr.bottom - 1 };
    ::DrawTextW(dc, d->query.empty() ? L"搜索" : d->query.c_str(), -1, &qr,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    const int listBottom = rc.top + rc.bottom - rc.top - kFpBtnH - 12;
    const int ltop = rc.top + kFpListTop;
    int vis = (listBottom - ltop) / kFpRowH;
    if (vis < 1) vis = 1;
    const int total = static_cast<int>(d->idxs.size());
    for (int shown = 0; shown < vis; ++shown)
    {
        const int logical = d->scroll + shown;
        if (logical > total) break;
        const int top = ltop + shown * kFpRowH;
        if (top + kFpRowH > listBottom) break;
        if (logical == 0)
        {
            bool on = true;
            for (int i = 0; i < total; ++i) if (!d->checked[d->idxs[i]]) { on = false; break; }
            DrawCheck(dc, rc.left + 12, top + 5, on);
            RECT tr = { rc.left + 32, top, rc.right - 8, top + kFpRowH };
            ::DrawTextW(dc, L"（全选）", -1, &tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
        else
        {
            const int vi = d->idxs[logical - 1];
            DrawCheck(dc, rc.left + 12, top + 5, d->checked[vi] != 0);
            RECT tr = { rc.left + 32, top, rc.right - 8, top + kFpRowH };
            const std::wstring text = d->vals[vi].empty() ? std::wstring(L"（空白）") : d->vals[vi];
            ::SetTextColor(dc, d->vals[vi].empty() ? RGB(0x8A, 0x94, 0xA6) : RGB(0x1F, 0x29, 0x37));
            ::DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }
    }

    HPEN ln = ::CreatePen(PS_SOLID, 1, RGB(0xE4, 0xE7, 0xEC));
    HGDIOBJ oldPen = ::SelectObject(dc, ln);
    ::MoveToEx(dc, rc.left, rc.bottom - kFpBtnH - 10, nullptr);
    ::LineTo(dc, rc.right, rc.bottom - kFpBtnH - 10);
    ::SelectObject(dc, oldPen);
    ::DeleteObject(ln);

    const int by1 = rc.bottom - kFpBtnH - 4, by0 = by1 - kFpBtnH;
    RECT okR = { rc.right - 8 - kFpBtnW, by0, rc.right - 8, by1 };
    RECT ccR = { okR.left - 8 - kFpBtnW, by0, okR.left - 8, by1 };
    HBRUSH okb = ::CreateSolidBrush(RGB(0x1F, 0x6F, 0xEB));
    ::FillRect(dc, &okR, okb); ::DeleteObject(okb);
    ::SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
    ::DrawTextW(dc, L"确定", -1, &okR, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    HBRUSH ccb = ::CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
    ::FillRect(dc, &ccR, ccb); ::DeleteObject(ccb);
    HPEN bp = ::CreatePen(PS_SOLID, 1, RGB(0xC9, 0xCE, 0xD6));
    ::SelectObject(dc, bp);
    ::Rectangle(dc, ccR.left, ccR.top, ccR.right, ccR.bottom);
    ::DeleteObject(bp);
    ::SetTextColor(dc, RGB(0x1F, 0x29, 0x37));
    ::DrawTextW(dc, L"取消", -1, &ccR, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    ::SelectObject(dc, oldFont);
    ::DeleteObject(font);
}

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

// ---------------------------- 列类型 ----------------------------
namespace
{
    std::wstring TrimCopy(const std::wstring& s)
    {
        size_t a = s.find_first_not_of(L" \t\r\n");
        if (a == std::wstring::npos) return L"";
        size_t b = s.find_last_not_of(L" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    bool ParseInt(const std::wstring& s, long long& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t.empty()) return false;
        wchar_t* end = nullptr;
        const long long v = ::_wcstoi64(t.c_str(), &end, 10);
        if (end == nullptr || *end != L'\0') return false;
        out = v;
        return true;
    }

    bool ParseDouble(const std::wstring& s, double& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t.empty()) return false;
        wchar_t* end = nullptr;
        const double v = ::wcstod(t.c_str(), &end);
        if (end == nullptr || *end != L'\0') return false;
        out = v;
        return true;
    }

    bool ParseBool(const std::wstring& s, bool& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t == L"1" || t == L"true" || t == L"True" || t == L"TRUE" || t == L"是") { out = true;  return true; }
        if (t == L"0" || t == L"false" || t == L"False" || t == L"FALSE" || t == L"否") { out = false; return true; }
        return false;
    }
}

void TableControl::SetColType(int col, ColType t)
{
    if (col < 0) return;
    if (static_cast<int>(m_colType.size()) <= col) m_colType.resize(col + 1, ColType::Text);
    m_colType[col] = t;
    Invalidate();
}

void TableControl::SetColTypes(const std::vector<int>& types)
{
    m_colType.assign(types.size(), ColType::Text);
    for (size_t i = 0; i < types.size(); ++i)
        m_colType[i] = static_cast<ColType>(types[i]);
    Invalidate();
}

ColType TableControl::ColTypeOf(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_colType.size())) return ColType::Text;
    return m_colType[col];
}

const std::wstring TableControl::GetText(int row, int col) const
{
    if (row < 0 || row >= Rows() || col < 0 || col >= static_cast<int>(m_rows[row].size()))
        return L"";
    return m_rows[row][col];
}

bool TableControl::GetInt(int row, int col, long long& out) const
{
    const std::wstring t = GetText(row, col);
    if (ParseInt(t, out)) return true;
    double d = 0.0;
    if (ParseDouble(t, d)) { out = static_cast<long long>(d); return true; }
    return false;
}

bool TableControl::GetDouble(int row, int col, double& out) const
{
    const std::wstring t = GetText(row, col);
    if (ParseDouble(t, out)) return true;
    long long i = 0;
    if (ParseInt(t, i)) { out = static_cast<double>(i); return true; }
    return false;
}

bool TableControl::GetBool(int row, int col, bool& out) const
{
    return ParseBool(GetText(row, col), out);
}

int TableControl::CompareCell(int r1, int c, int r2) const
{
    const ColType t = ColTypeOf(c);
    if (t == ColType::Int)
    {
        long long a = 0, b = 0;
        if (GetInt(r1, c, a) && GetInt(r2, c, b)) return (a < b) ? -1 : (a > b ? 1 : 0);
    }
    else if (t == ColType::Double)
    {
        double a = 0, b = 0;
        if (GetDouble(r1, c, a) && GetDouble(r2, c, b)) return (a < b) ? -1 : (a > b ? 1 : 0);
    }
    else if (t == ColType::Bool)
    {
        bool a = false, b = false;
        if (GetBool(r1, c, a) && GetBool(r2, c, b)) return (a == b) ? 0 : (a ? 1 : -1);
    }
    const std::wstring x = GetText(r1, c), y = GetText(r2, c);
    return (x < y) ? -1 : (x > y ? 1 : 0);
}

void TableControl::AppendRow(const std::vector<std::wstring>& cells)
{
    m_rows.push_back(cells);
    // 逐行追加时不能整表重建视图（10221 行会退化成 O(n²) 卡好几秒）
    const int r = static_cast<int>(m_rows.size()) - 1;
    bool anyFilter = false;
    for (int c = 0; c < Cols(); ++c)
    {
        if ((c < static_cast<int>(m_filter.size()) && !m_filter[c].empty()) ||
            (c < static_cast<int>(m_filterValues.size()) && !m_filterValues[c].empty()))
        { anyFilter = true; break; }
    }
    if (!anyFilter) m_view.push_back(r);
    else if (RowPassesFilter(cells)) m_view.push_back(r);
    Invalidate();
}

void TableControl::SetRowsBulk(const std::vector<std::vector<std::wstring>>& rows)
{
    // 整表一次性灌入：不要逐行 Invalidate（10221 行会发出上万次重绘请求）
    m_rows.clear();
    m_rows.reserve(rows.size());
    m_view.clear();
    m_view.reserve(rows.size());
    bool anyFilter = false;
    for (int c = 0; c < Cols(); ++c)
    {
        if ((c < static_cast<int>(m_filter.size()) && !m_filter[c].empty()) ||
            (c < static_cast<int>(m_filterValues.size()) && !m_filterValues[c].empty()))
        { anyFilter = true; break; }
    }
    for (size_t r = 0; r < rows.size(); ++r)
    {
        m_rows.push_back(rows[r]);
        if (!anyFilter) m_view.push_back(static_cast<int>(r));
        else if (RowPassesFilter(rows[r])) m_view.push_back(static_cast<int>(r));
    }
    if (m_sel >= static_cast<int>(m_view.size())) m_sel = -1;
    m_scroll = 0;
    Invalidate();
}

bool TableControl::RowPassesFilter(const std::vector<std::wstring>& cells) const
{
    for (int c = 0; c < Cols(); ++c)
    {
        const std::wstring cell = (c < static_cast<int>(cells.size())) ? cells[c] : std::wstring();
        if (c < static_cast<int>(m_filterValues.size()) && !m_filterValues[c].empty())
        {
            bool hit = false;
            for (const auto& v : m_filterValues[c]) if (v == cell) { hit = true; break; }
            if (!hit) return false;
        }
        else if (c < static_cast<int>(m_filter.size()) && !m_filter[c].empty())
        {
            if (!ContainsNoCase(cell, m_filter[c])) return false;
        }
    }
    return true;
}

// ------------------------- 过滤视图 / 排序 -------------------------
void TableControl::RebuildView()
{
    m_view.clear();
    if (static_cast<int>(m_filter.size()) < Cols()) m_filter.resize(Cols());
    if (static_cast<int>(m_filterValues.size()) < Cols()) m_filterValues.resize(Cols());
    for (int r = 0; r < static_cast<int>(m_rows.size()); ++r)
    {
        bool keep = true;
        for (int c = 0; c < Cols() && keep; ++c)
        {
            const std::wstring cell = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : std::wstring();
            if (c < static_cast<int>(m_filterValues.size()) && !m_filterValues[c].empty())
            {
                bool hit = false;
                for (const auto& v : m_filterValues[c]) if (v == cell) { hit = true; break; }
                if (!hit) keep = false;
            }
            else if (c < static_cast<int>(m_filter.size()) && !m_filter[c].empty())
            {
                if (!ContainsNoCase(cell, m_filter[c])) keep = false;
            }
        }
        if (keep) m_view.push_back(r);
    }
}

int TableControl::ViewPos(int dataRow) const
{
    for (size_t i = 0; i < m_view.size(); ++i) if (m_view[i] == dataRow) return static_cast<int>(i);
    return -1;
}

int TableControl::ColAtX(int x) const
{
    int acc = -m_colScroll;
    for (int i = 0; i < Cols(); ++i)
    {
        if (x >= acc && x < acc + m_colW[i]) return i;
        acc += m_colW[i];
    }
    return -1;
}

void TableControl::SetFilter(int col, const std::wstring& text)
{
    if (col < 0) return;
    if (static_cast<int>(m_filter.size()) <= col) m_filter.resize(col + 1);
    m_filter[col] = text;
    RebuildView();
    Invalidate();
}

std::wstring TableControl::FilterOf(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_filter.size())) return L"";
    return m_filter[col];
}

void TableControl::ClearFilters()
{
    if (static_cast<int>(m_filter.size()) < Cols()) m_filter.resize(Cols());
    if (static_cast<int>(m_filterValues.size()) < Cols()) m_filterValues.resize(Cols());
    for (int c = 0; c < Cols(); ++c) { m_filter[c].clear(); m_filterValues[c].clear(); }
    RebuildView();
    Invalidate();
}

void TableControl::SortBy(int col, bool desc)
{
    if (col < 0 || col >= Cols()) return;
    const long long savedSel = (m_sel >= 0 && m_sel < Rows()) ? m_sel : -1;
    std::stable_sort(m_view.begin(), m_view.end(), [&](int a, int b) {
        const int cmp = CompareCell(a, col, b);
        return desc ? cmp > 0 : cmp < 0;
    });
    if (m_view.empty()) { RebuildView(); }
    (void)savedSel;
    Invalidate();
}

void TableControl::ApplySort(int col, bool desc) { SortBy(col, desc); }

// 点漏斗：在表头格上临时开一个 EDIT，模态收字，回车应用 / Esc 取消
// ---------------- 表头漏斗：Excel 式下拉选值（画在表格自己身上） ----------------
void TableControl::OpenFilterPopup(int col)
{
    if (col < 0 || col >= Cols()) return;
    CloseFilterPopup();

    int left = 0;
    for (int i = 0; i < col; ++i) left += m_colW[i];

    FilterPopupData* d = new FilterPopupData();
    d->owner = this;
    d->col = col;
    d->vals = DistinctValues(this, col);
    d->checked.assign(d->vals.size(), 1);
    if (col < static_cast<int>(m_filterValues.size()) && !m_filterValues[col].empty())
    {
        for (auto& c : d->checked) c = 0;
        for (const auto& v : m_filterValues[col])
            for (int i = 0; i < static_cast<int>(d->vals.size()); ++i)
                if (d->vals[i] == v) d->checked[i] = 1;
    }

    RefreshFiltered(d);          // 打开时先把整列去重值填进候选列表（不填就只有「全选」一行）
    const RECT rc = Rect();
    const int w = 250;
    int x = left + m_colW[col] - 20;
    if (x + w > rc.right) x = rc.right - w - 2;
    if (x < 0) x = 0;
    const int rows = std::min(12, static_cast<int>(d->vals.size()) + 1);
    int h = kFpListTop + rows * kFpRowH + kFpBtnH + 16;
    if (h > rc.bottom - m_headerH - 4) h = rc.bottom - m_headerH - 4;
    if (h < 150) h = 150;
    d->rc.left = x; d->rc.top = m_headerH + 1; d->rc.right = x + w; d->rc.bottom = m_headerH + 1 + h;

    m_fp = d;
    ::SetFocus(m_hwnd);
    Invalidate();
}

void TableControl::ApplyFilterPopup(FilterPopupData* d)
{
    if (d == nullptr) return;
    std::vector<std::wstring> sel;
    bool all = true;
    for (int i = 0; i < static_cast<int>(d->checked.size()); ++i)
        if (!d->checked[i]) { all = false; break; }
    if (!all)
        for (int i = 0; i < static_cast<int>(d->checked.size()); ++i)
            if (d->checked[i]) sel.push_back(d->vals[i]);
    d->owner->SetFilterValues(d->col, sel);
    d->owner->CloseFilterPopup();
}

void TableControl::CloseFilterPopup()
{
    if (m_fp == nullptr) return;
    FilterPopupData* d = m_fp;
    m_fp = nullptr;
    delete d;
    Invalidate();
}

bool TableControl::HandleFilterPanelClick(int mx, int my)
{
    if (m_fp == nullptr) return false;
    FilterPopupData* d = m_fp;
    const RECT& rc = d->rc;
    if (mx < rc.left || mx >= rc.right || my < rc.top || my >= rc.bottom) return false;
    const int x = mx - rc.left, y = my - rc.top;
    const int ch = rc.bottom - rc.top, cw = rc.right - rc.left;
    const int by1 = ch - kFpBtnH - 4, by0 = by1 - kFpBtnH;
    if (y >= by0 && y <= by1)
    {
        RECT okR = { cw - 8 - kFpBtnW, by0, cw - 8, by1 };
        RECT ccR = { okR.left - 8 - kFpBtnW, by0, okR.left - 8, by1 };
        if (x >= okR.left && x <= okR.right) { ApplyFilterPopup(d); return true; }
        if (x >= ccR.left && x <= ccR.right) { CloseFilterPopup(); return true; }
        return true;
    }
    if (y < kFpListTop) return true;                       // 搜索框区域，键盘输入即可
    const int listBottom = ch - kFpBtnH - 12;
    if (y >= listBottom) return true;
    const int logical = d->scroll + (y - kFpListTop) / kFpRowH;
    if (logical < 0 || logical > static_cast<int>(d->idxs.size())) return true;
    if (logical == 0)
    {
        // 「全选」按整列所有候选值算：已有筛选(query)时，勾/取消都要覆盖被搜索词挡掉的那些值，
        // 否则 搜索→取消全选→只勾一项 会变成「全都勾着」而等于没有筛选。
        bool all = true;
        for (int i = 0; i < static_cast<int>(d->checked.size()); ++i) if (!d->checked[i]) { all = false; break; }
        for (int i = 0; i < static_cast<int>(d->checked.size()); ++i) d->checked[i] = all ? 0 : 1;
    }
    else
    {
        const int vi = d->idxs[logical - 1];
        d->checked[vi] = d->checked[vi] ? 0 : 1;
    }
    ::InvalidateRect(m_hwnd, &rc, FALSE);
    return true;
}

bool TableControl::HandleFilterPanelKey(UINT msg, WPARAM wp)
{
    if (m_fp == nullptr) return false;
    FilterPopupData* d = m_fp;
    if (msg == WM_CHAR)
    {
        if (wp == 8)                                       // 退格
        {
            if (!d->query.empty()) d->query.pop_back();
        }
        else if (wp >= 32 && wp < 0x10000)
        {
            d->query.push_back(static_cast<wchar_t>(wp));
        }
        d->scroll = 0;
        RefreshFiltered(d);
        ::InvalidateRect(m_hwnd, &d->rc, FALSE);
        return true;
    }
    if (msg == WM_KEYDOWN)
    {
        if (wp == VK_ESCAPE) { CloseFilterPopup(); return true; }
        if (wp == VK_RETURN) { ApplyFilterPopup(d); return true; }
        if (wp == VK_BACK)
        {
            if (!d->query.empty()) d->query.pop_back();
            d->scroll = 0;
            RefreshFiltered(d);
            ::InvalidateRect(m_hwnd, &d->rc, FALSE);
            return true;
        }
        if (wp == VK_DOWN || wp == VK_UP)
        {
            d->scroll += (wp == VK_DOWN ? 1 : -1);
            if (d->scroll < 0) d->scroll = 0;
            const int n = static_cast<int>(d->idxs.size());
            if (d->scroll > n) d->scroll = n;
            ::InvalidateRect(m_hwnd, &d->rc, FALSE);
            return true;
        }
        return true;                                       // 面板开着时吃掉其它键，别触发 F2/Del
    }
    return false;
}

void TableControl::SetFilterValues(int col, const std::vector<std::wstring>& values)
{
    if (col < 0) return;
    if (static_cast<int>(m_filterValues.size()) < Cols()) m_filterValues.resize(Cols());
    m_filterValues[col] = values;
    if (static_cast<int>(m_filter.size()) < Cols()) m_filter.resize(Cols());
    m_filter[col].clear();                 // 值筛选与子串筛选互斥
    RebuildView();
    Invalidate();
}

void TableControl::SetFilterValue(int col, const std::wstring& value)
{
    std::vector<std::wstring> v;
    if (!value.empty()) v.push_back(value);
    SetFilterValues(col, v);
}

std::vector<std::wstring> TableControl::FilterValuesOf(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_filterValues.size())) return std::vector<std::wstring>();
    return m_filterValues[col];
}

std::vector<std::wstring> TableControl::RowAt(int row) const
{
    if (row < 0 || row >= Rows()) return std::vector<std::wstring>();
    return m_rows[row];
}

std::wstring TableControl::ColumnTitle(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_colTitle.size())) return L"";
    return m_colTitle[col];
}

// 右键表头：弹出菜单（升序 / 降序 / 清本列筛选 / 清全部筛选）
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
}

void TableControl::Clear() { CloseFilterPopup(); m_rows.clear(); m_sel = -1; m_view.clear(); Invalidate(); }

void TableControl::SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles)
{
    m_colW = widths;
    m_colTitle = titles;
    m_colType.assign(widths.size(), ColType::Text);   // 未指定前一律文本
    Invalidate();
}

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

int TableControl::VisibleCount() const
{
    const RECT rc = Rect();
    const int hb = HasHBar() ? kSbH : 0;
    const int viewH = rc.bottom - rc.top - m_headerH - hb;
    return viewH > 0 ? viewH / m_rowH : 1;
}

int TableControl::MaxScroll() const
{
    const int vis = VisibleCount();
    const int all = ViewRows();
    return all > vis ? all - vis : 0;
}

RECT TableControl::RowRect(int row) const
{
    RECT rc = { 0,0,0,0 };
    const int pos = ViewPos(row) - m_scroll;
    if (pos < 0) return rc;
    rc.left = 0;
    rc.top = m_headerH + pos * m_rowH;
    rc.right = Rect().right;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

RECT TableControl::Cell(int row, int col) const
{
    RECT rc = { 0,0,0,0 };
    if (row < 0 || row >= Rows() || col < 0 || col >= Cols()) return rc;
    int x = -m_colScroll;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    rc.left = x;
    rc.right = x + m_colW[col];
    rc.top = m_headerH + (ViewPos(row) - m_scroll) * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}

bool TableControl::Hit(int x, int y, int& row, int& col) const
{
    row = -1; col = -1;
    if (y < m_headerH) return false;
    const int pos = m_scroll + (y - m_headerH) / m_rowH;
    if (pos < 0 || pos >= ViewRows()) return false;
    row = m_view[pos];                       // 可视位置 -> 数据行（过滤后仍对得上）
    col = ColAtX(x);
    return col >= 0;
}

// ---- 自绘纵向滚动条几何：绘制与命中用同一份算法 ----
RECT TableControl::SbTrack() const
{
    const RECT rc = Rect();
    const int sbW = 10;
    const int hb = HasHBar() ? kSbH : 0;
    RECT t = { rc.right - sbW, rc.top + m_headerH, rc.right, rc.bottom - hb };
    return t;
}

void TableControl::SbThumb(int& thumbTop, int& thumbH) const
{
    const RECT tr = SbTrack();
    const int travel = tr.bottom - tr.top;
    const int mx = MaxScroll();
    const int all = ViewRows();
    thumbTop = tr.top;
    thumbH = travel;
    if (mx <= 0 || all <= 0 || travel <= 0)
        return;
    thumbH = travel * VisibleCount() / all;
    if (thumbH < 24) thumbH = 24;
    if (thumbH > travel) thumbH = travel;
    const int span = travel - thumbH;
    thumbTop = tr.top + (span > 0 ? m_scroll * span / mx : 0);
}

int TableControl::SbScrollFromY(int y, int thumbH) const
{
    const RECT tr = SbTrack();
    const int mx = MaxScroll();
    const int span = (tr.bottom - tr.top) - thumbH;
    if (mx <= 0 || span <= 0)
        return 0;
    int v = (y - tr.top) * mx / span;
    if (v < 0) v = 0;
    if (v > mx) v = mx;
    return v;
}

// ---- 自绘横向滚动条几何：与纵向同一套口径 ----

int TableControl::ContentW() const
{
    int w = 0;
    for (int i = 0; i < static_cast<int>(m_colW.size()); ++i)
        w += m_colW[i];
    return w;
}

bool TableControl::HasHBar() const
{
    const RECT rc = Rect();
    return ContentW() > rc.right - 10;      // 右侧留 10px 给纵条
}

int TableControl::MaxHScroll() const
{
    const RECT rc = Rect();
    const int span = rc.right - 10;
    const int mx = ContentW() - span;
    return mx > 0 ? mx : 0;
}

void TableControl::ScrollXBy(int px)
{
    const int mx = MaxHScroll();
    m_colScroll += px;
    if (m_colScroll < 0) m_colScroll = 0;
    if (m_colScroll > mx) m_colScroll = mx;
    Invalidate();
}

RECT TableControl::SbTrackX() const
{
    const RECT rc = Rect();
    RECT t = { rc.left, rc.bottom - kSbH, rc.right - 10, rc.bottom };
    return t;
}

void TableControl::SbThumbX(int& thumbLeft, int& thumbW) const
{
    const RECT tr = SbTrackX();
    const int travel = tr.right - tr.left;
    const int mx = MaxHScroll();
    thumbLeft = tr.left;
    thumbW = travel;
    if (mx <= 0 || travel <= 0)
        return;
    const int content = ContentW();
    thumbW = travel * (content - mx) / content;
    if (thumbW < 40) thumbW = 40;
    if (thumbW > travel) thumbW = travel;
    const int span = travel - thumbW;
    thumbLeft = tr.left + (span > 0 ? m_colScroll * span / mx : 0);
}

int TableControl::SbScrollFromX(int x, int thumbW) const
{
    const RECT tr = SbTrackX();
    const int mx = MaxHScroll();
    const int span = (tr.right - tr.left) - thumbW;
    if (mx <= 0 || span <= 0)
        return 0;
    int v = (x - tr.left) * mx / span;
    if (v < 0) v = 0;
    if (v > mx) v = mx;
    return v;
}

void TableControl::ScrollBy(int rows){
    const int mx = MaxScroll();
    m_scroll = m_scroll + rows;
    if (m_scroll < 0) m_scroll = 0;
    if (m_scroll > mx) m_scroll = mx;
    InvalidateScrollOnly();
}

void TableControl::EnsureVisible(int row)
{
    if (row < 0 || row >= Rows()) return;
    const int vis = VisibleCount();
    if (row < m_scroll)
        m_scroll = row;
    else if (row >= m_scroll + vis)
        m_scroll = row - vis + 1;
    const int mx = MaxScroll();
    if (m_scroll < 0) m_scroll = 0;
    if (m_scroll > mx) m_scroll = mx;
    InvalidateScrollOnly();
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
{
    // 数据/行列/选中变了：离屏内容带作废，下一帧重建
    m_ctxDirty = true;
    if (m_hwnd && ::IsWindow(m_hwnd))
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
}

// 只是纵向滚动位置变了：缓冲里的内容还有效，交给 OnPaint 走"平移 + 补一条"
void TableControl::ClampXScroll()
{
    const int mx = MaxHScroll();
    if (m_colScroll > mx) m_colScroll = mx;
    if (m_colScroll < 0) m_colScroll = 0;
}

void TableControl::InvalidateScrollOnly()
{
    if (m_hwnd && ::IsWindow(m_hwnd))
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
}

void TableControl::Fire(int row, int col, int what, const std::wstring& text)
{ m_pendingText = text; if (m_fn) m_fn(m_ctx, row, col, what); }

WNDPROC TableControl::s_headerEditProc = nullptr;

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
    case kMsgHeaderFilter:
        self->OpenFilterPopup(static_cast<int>(wp));
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        if (self->m_fp != nullptr && !self->HandleFilterPanelClick(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)))
            self->CloseFilterPopup();                                     // 点面板外 = 关掉漏斗面板
        // 底部横向滚动条：按在拇指上接着拖，按在空轨道直接跳
        if (self->HasHBar())
        {
            const RECT rcNow = self->Rect();
            const int px = GET_X_LPARAM(lp), py = GET_Y_LPARAM(lp);
            if (py >= rcNow.bottom - kSbH && px < rcNow.right - 10)
            {
                int thL = 0, thW = 0;
                self->SbThumbX(thL, thW);
                self->m_hsbGrab = thW / 2;
                if (px >= thL && px < thL + thW)
                    self->m_hsbGrab = px - thL;
                self->m_sbHDrag = 1;
                self->m_colScroll = self->SbScrollFromX(px - self->m_hsbGrab, thW);
                self->Invalidate();
                ::SetCapture(hwnd);
                return 0;
            }
        }
        // 自绘滚动条：按在拇指上 = 接着拖；按在空轨道 = 直接跳到该位置
        {
            const RECT rcNow = self->Rect();
            const int px = GET_X_LPARAM(lp), py = GET_Y_LPARAM(lp);
            if (py >= self->m_headerH && py < rcNow.bottom - (self->HasHBar() ? kSbH : 0) &&
                px >= rcNow.right - 10)
            {
                int thumbY = 0, thumbH = 0;
                self->SbThumb(thumbY, thumbH);
                self->m_sbGrab = thumbH / 2;
                if (py >= thumbY && py < thumbY + thumbH)
                    self->m_sbGrab = py - thumbY;      // 抓在拇指中间就别跳
                self->m_sbDrag = 1;
                self->m_scroll = self->SbScrollFromY(py - self->m_sbGrab, thumbH);
                self->InvalidateScrollOnly();
                ::SetCapture(hwnd);
                return 0;
            }
        }
        // 表头：① 落在列右边界 ±3px 内 = 拖列宽
        //       ② 列名右端漏斗区（宽 18px）= 弹筛选框（用 PostMessage 延后执行，避免在按键处理里跑模态循环）
        //       ③ 其余位置单击列名 = 按该列排序
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int mx = GET_X_LPARAM(lp);
            int acc = 0, grab = -1;
            for (int i = 0; i + 1 < static_cast<int>(self->m_colW.size()); ++i)
            {
                acc += self->m_colW[i];
                if (mx >= acc - 3 && mx <= acc + 3) { grab = i; break; }
            }
            if (grab >= 0)
            {
                self->m_resizeCol = grab;
                self->m_resizeX0 = mx;
                self->m_resizeW0 = self->m_colW[grab];
                ::SetCapture(hwnd);
            }
            else
            {
                const int col = self->ColAtX(mx);
                if (col >= 0 && col < static_cast<int>(self->m_colW.size()))
                {
                    int left = 0;
                    for (int i = 0; i < col; ++i) left += self->m_colW[i];
                    const int right = left + self->m_colW[col];
                    // 漏斗区收到 right-4 为止：右侧 3px 留给"拖列宽"，两者不再重叠
                    if (mx >= right - 20 && mx < right - 3)
                        self->OpenFilterPopup(col);        // 漏斗：弹出 Excel 式选值面板
                    else
                        self->Fire(-1, col, 8);            // 8 = 点表头 → 按该列排序
                }
            }
            return 0;
        }
        int row = -1, col = -1;
        if (self->Hit(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), row, col))
        {
            if (row != self->m_sel) { self->Select(row); self->Fire(row, col, 3); }
            self->Fire(row, col, msg == WM_LBUTTONDBLCLK ? 2 : 1);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (self->m_sbDrag)
        {
            int thumbY = 0, thumbH = 0;
            self->SbThumb(thumbY, thumbH);
            self->m_scroll = self->SbScrollFromY(GET_Y_LPARAM(lp) - self->m_sbGrab, thumbH);
            self->InvalidateScrollOnly();
            return 0;
        }
        if (self->m_sbHDrag)
        {
            int thL = 0, thW = 0;
            self->SbThumbX(thL, thW);
            self->m_colScroll = self->SbScrollFromX(GET_X_LPARAM(lp) - self->m_hsbGrab, thW);
            self->Invalidate();
            return 0;
        }
        if (self->m_resizeCol >= 0)
        {
            const int w = self->m_resizeW0 + (GET_X_LPARAM(lp) - self->m_resizeX0);
            if (w >= 28 && self->m_resizeCol < static_cast<int>(self->m_colW.size()))
                self->m_colW[self->m_resizeCol] = w;   // 最少 28px，别拖成负数
            self->Invalidate();
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (self->m_sbDrag)
        {
            self->m_sbDrag = 0;
            ::ReleaseCapture();
            return 0;
        }
        if (self->m_sbHDrag)
        {
            self->m_sbHDrag = 0;
            ::ReleaseCapture();
            return 0;
        }
        if (self->m_resizeCol >= 0)
        {
            self->m_resizeCol = -1;
            ::ReleaseCapture();
            return 0;
        }
        break;
    }
    case WM_SETCURSOR:
    {
        // 鼠标在表头列边界上给左右箭头，让人知道这里能拖
        if (LOWORD(lp) == HTCLIENT)
        {
            POINT pt;
            ::GetCursorPos(&pt);
            ::ScreenToClient(hwnd, &pt);
            int acc = 0, hit = -1;
            if (pt.y < self->m_headerH)
            {
                for (int i = 0; i + 1 < static_cast<int>(self->m_colW.size()); ++i)
                {
                    acc += self->m_colW[i];
                    if (pt.x >= acc - 4 && pt.x <= acc + 4) { hit = i; break; }
                }
            }
            if (hit >= 0)
            {
                ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEWE));
                return TRUE;
            }
            const RECT rcNow = self->Rect();
            if (self->HasHBar() && pt.y >= rcNow.bottom - kSbH && pt.x < rcNow.right - 10)
            {
                ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    }
    case WM_RBUTTONDOWN:
    {
        // 右键表头：弹出菜单（升序 / 降序 / 清除该列筛选 / 清除全部筛选）
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int col = self->ColAtX(GET_X_LPARAM(lp));
            if (col >= 0) self->RunHeaderMenu(col);
            return 0;
        }
        break;
    }
    case WM_CHAR:
        if (self->HandleFilterPanelKey(WM_CHAR, wp)) return 0;
        break;
    case WM_KEYDOWN:
    {
        if (self->HandleFilterPanelKey(WM_KEYDOWN, wp)) return 0;
        if (wp == VK_F2)      { self->Fire(self->m_sel, 0, 4); return 0; }
        if (wp == VK_DELETE)  { self->Fire(self->m_sel, 0, 5); return 0; }
        if (wp == VK_INSERT)  { self->Fire(self->m_sel, 0, 6); return 0; }
        // 表格控件本身不可输入文本，字母键只承担快捷键；复制/粘贴不依赖 GetKeyState
        // （无人值守会话里真实 keybd_event 的 Ctrl 状态常读不到）。
        if (wp == 'C' || wp == 'c')
        {
            if (self->m_sel >= 0 && self->m_sel < static_cast<int>(self->m_rows.size()))
            {
                std::wstring s;
                for (size_t c = 0; c < self->m_rows[self->m_sel].size(); ++c)
                {
                    if (c) s += L'\t';
                    s += self->m_rows[self->m_sel][c];
                }
                setClipboardText(self->m_hwnd, s);
            }
            return 0;
        }
        if (wp == 'V' || wp == 'v')
        {
            self->Fire(self->m_sel, 0, 7, getClipboardText(self->m_hwnd));
            return 0;
        }
        if (wp == VK_DOWN || wp == VK_UP)
        {
            const int next = self->m_sel < 0 ? 0 : self->m_sel + (wp == VK_DOWN ? 1 : -1);
            self->Select(next);
            self->Fire(self->m_sel, 0, 3);
            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        const short delta = GET_WHEEL_DELTA_WPARAM(wp);
        const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shift)
            self->ScrollXBy(delta > 0 ? -60 : 60);   // Shift+滚轮 = 横向（Excel 习惯）
        else
            self->ScrollBy(delta > 0 ? -1 : 1);      // 滚轮向上 = 内容上滚，后行下移
        return 0;
    }
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

void TableControl::ReleaseGdi()
{
    if (m_font != nullptr) { ::DeleteObject(m_font); m_font = nullptr; }
    if (m_brHdr != nullptr) { ::DeleteObject(m_brHdr); m_brHdr = nullptr; }
    if (m_brSel != nullptr) { ::DeleteObject(m_brSel); m_brSel = nullptr; }
    if (m_brLine != nullptr) { ::DeleteObject(m_brLine); m_brLine = nullptr; }
    if (m_brTrack != nullptr) { ::DeleteObject(m_brTrack); m_brTrack = nullptr; }
    if (m_brThumb != nullptr) { ::DeleteObject(m_brThumb); m_brThumb = nullptr; }
    if (m_memBmp != nullptr) { ::DeleteObject(m_memBmp); m_memBmp = nullptr; }
    if (m_memDc != nullptr) { ::DeleteDC(m_memDc); m_memDc = nullptr; }
    if (m_ctxBmp != nullptr) { ::DeleteObject(m_ctxBmp); m_ctxBmp = nullptr; }
    if (m_ctxDc != nullptr) { ::DeleteDC(m_ctxDc); m_ctxDc = nullptr; }
    m_memW = m_memH = 0;
}

// 离屏缓冲按"只增不减"缓存：客户区尺寸不变时一直复用同一张 DIB + 同一个 DC，
// 滚动也就只剩 FillRect + DrawText 两件事。
bool TableControl::EnsureBuffers(const RECT& rc)
{
    if (rc.right <= 0 || rc.bottom <= 0)
        return false;
    if (m_memDc != nullptr && m_memBmp != nullptr && m_memW >= rc.right && m_memH >= rc.bottom)
        return true;

    const int nw = rc.right > m_memW ? rc.right : m_memW;
    const int nh = rc.bottom > m_memH ? rc.bottom : m_memH;
    if (m_memBmp != nullptr) { ::DeleteObject(m_memBmp); m_memBmp = nullptr; }
    if (m_memDc != nullptr) { ::DeleteDC(m_memDc); m_memDc = nullptr; }
    if (m_ctxBmp != nullptr) { ::DeleteObject(m_ctxBmp); m_ctxBmp = nullptr; }
    if (m_ctxDc != nullptr) { ::DeleteDC(m_ctxDc); m_ctxDc = nullptr; }

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = nw;
    bi.bmiHeader.biHeight = -nh;              // 自顶向下，和屏幕坐标一致
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC dc = ::CreateCompatibleDC(nullptr);
    if (bmp == nullptr || dc == nullptr)
    {
        if (bmp != nullptr) ::DeleteObject(bmp);
        if (dc != nullptr) ::DeleteDC(dc);
        return false;
    }
    ::SelectObject(dc, bmp);
    m_memBmp = bmp;
    m_memDc = dc;
    m_memW = nw;
    m_memH = nh;
    return true;
}

LRESULT TableControl::OnPaint()
{
    PAINTSTRUCT ps = {};
    HDC dcScreen = ::BeginPaint(m_hwnd, &ps);
    const RECT rc = Rect();

    if (!EnsureBuffers(rc))
    {
        ::EndPaint(m_hwnd, &ps);
        return 0;
    }
    HDC dc = m_memDc;
    ::FillRect(dc, &rc, ::GetSysColorBrush(COLOR_WINDOW));

    const int logPx = ::MulDiv(m_pt, ::GetDeviceCaps(dc, LOGPIXELSY), 72);
    if (m_font == nullptr || m_fontPt != logPx || m_fontFace != m_face)
    {
        if (m_font != nullptr) { ::DeleteObject(m_font); m_font = nullptr; }
        m_font = ::CreateFontW(-logPx, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, m_face.c_str());
        m_fontPt = logPx;
        m_fontFace = m_face;
    }
    HGDIOBJ oldFont = (m_font != nullptr) ? ::SelectObject(dc, m_font) : nullptr;
    ::SetBkMode(dc, TRANSPARENT);

    // 画刷同样按颜色缓存（同一支刷子反复用，不再每行建一支）
    auto brushOf = [](HBRUSH& br, COLORREF& cached, COLORREF want) -> HBRUSH {
        if (br != nullptr && cached == want) return br;
        if (br != nullptr) ::DeleteObject(br);
        br = ::CreateSolidBrush(want);
        cached = want;
        return br;
    };

    // 表头
    RECT hdr = { rc.left, rc.top, rc.right, rc.top + m_headerH };
    ::FillRect(dc, &hdr, brushOf(m_brHdr, m_brHdrC, m_hdrBg));
    ::SetTextColor(dc, m_fg);
    int x = -m_colScroll;
    for (int i = 0; i < Cols(); ++i)
    {
        if (x >= rc.right)
            break;
        if (x + m_colW[i] <= 0) { x += m_colW[i]; continue; }        // 整列被推到左边看不见
        RECT cellRc = { x + 6, hdr.top, x + m_colW[i] - 12, hdr.bottom };   // 右留 12px 给漏斗
        const std::wstring t = i < static_cast<int>(m_colTitle.size()) ? m_colTitle[i] : L"";
        const ColType ct = ColTypeOf(i);
        const UINT align = (ct == ColType::Int || ct == ColType::Double) ? DT_RIGHT
                        : (ct == ColType::Bool ? DT_CENTER : DT_LEFT);
        ::DrawTextW(dc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 列名右侧常驻漏斗按钮（Excel 那种）：点它弹输入框筛本列；
        // 该列有筛选词时漏斗变蓝，再点一次（输空回车）即取消筛选。
        {
            const int fx = x + m_colW[i] - 11;    // x 已含 -m_colScroll
            const int fy = hdr.top + (m_headerH - 10) / 2;
            const bool on = !FilterOf(i).empty() ||
                            (i < static_cast<int>(m_filterValues.size()) && !m_filterValues[i].empty());
            const COLORREF fc = on ? RGB(0x1F, 0x6F, 0xEB) : RGB(0x8A, 0x94, 0xA6);
            ::SelectObject(dc, ::GetStockObject(NULL_PEN));
            HBRUSH fb = ::CreateSolidBrush(fc);
            HGDIOBJ ob = ::SelectObject(dc, fb);
            // 梯形漏斗：上宽下窄 + 一根短柄
            POINT pts[4] = { { fx, fy }, { fx + 8, fy }, { fx + 5, fy + 4 }, { fx + 3, fy + 4 } };
            ::Polygon(dc, pts, 4);
            RECT stem = { fx + 3, fy + 4, fx + 6, fy + 8 };
            ::FillRect(dc, &stem, fb);
            ::SelectObject(dc, ob);
            ::DeleteObject(fb);
        }
        x += m_colW[i];
    }

    // 行内容缓存：整表所有行先画进一张离屏"内容带"，之后无论滚多少行，
    // 上屏只是 BitBlt 一段切片 —— DrawTextW 的代价只在数据变化时付一次。
    const int hbH = HasHBar() ? kSbH : 0;
    const RECT contentBand = { rc.left, rc.top + m_headerH, rc.right, rc.bottom - hbH };
    const int contentH = contentBand.bottom - contentBand.top;
    const int rows = ViewRows();

    auto paintRow = [&](HDC tdc, int r, int yTop) {
        RECT rr = { 0, yTop, rc.right, yTop + m_rowH };
        if (r == m_sel)
        {
            ::FillRect(tdc, &rr, brushOf(m_brSel, m_brSelC, m_selBg));
            ::SetTextColor(tdc, RGB(0xFF, 0xFF, 0xFF));
        }
        else ::SetTextColor(tdc, m_fg);

        int cx = -m_colScroll;
        for (int c = 0; c < Cols(); ++c)
        {
            if (cx >= rc.right)
                break;                             // 窗口变窄时右侧列不画
            if (cx + m_colW[c] <= 0) { cx += m_colW[c]; continue; }
            RECT cellRc = { cx + 6, rr.top, cx + m_colW[c] - 4, rr.bottom };
            const std::wstring t = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : L"";
            const ColType ct = ColTypeOf(c);
            const UINT align = (ct == ColType::Int || ct == ColType::Double) ? DT_RIGHT
                            : (ct == ColType::Bool ? DT_CENTER : DT_LEFT);
            ::DrawTextW(tdc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            cx += m_colW[c];
        }
        RECT lineRc = { 0, rr.bottom - 1, rc.right, rr.bottom };
        ::FillRect(tdc, &lineRc, brushOf(m_brLine, m_brLineC, m_line));
    };

    // 内容带只缓存"当前滚动位置上下各 vis 行"这一段，滚出这段才重建：
    // 表再大（12000 行）每次滚动也只是 BitBlt 一条切片，不会退化成逐行 DrawText。
    const int visRows = (m_rowH > 0) ? (contentH + m_rowH - 1) / m_rowH : 0;
    const int bandRows = visRows > 0 ? visRows * 6 : 0;
    const int contentTotal = rows * m_rowH;
    const bool cacheOk = bandRows > 0 && rows > 0;

    if (cacheOk)
    {
        const int wantFrom = std::max(0, std::min(m_scroll - visRows, rows - bandRows));
        const int argFrom = std::max(0, std::min(m_scroll, rows - bandRows));
        const int bandSpan = std::min(bandRows, rows);
        const bool outside = m_scroll < m_ctxFrom || m_scroll + visRows > m_ctxFrom + m_ctxRows;

        if (m_ctxDirty || m_ctxDc == nullptr || m_ctxW < rc.right || outside || m_ctxX != m_colScroll)
        {
            const int from = outside ? argFrom : std::max(0, std::min(m_ctxFrom, rows - bandSpan));
            (void)wantFrom;
            const int nRows = std::min(bandRows, rows - from);
            const int needH = nRows * m_rowH;
            if (m_ctxBmp == nullptr || m_ctxH < needH || m_ctxW < rc.right)
            {
                if (m_ctxBmp != nullptr) { ::DeleteObject(m_ctxBmp); m_ctxBmp = nullptr; }
                if (m_ctxDc != nullptr) { ::DeleteDC(m_ctxDc); m_ctxDc = nullptr; }
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = rc.right;
                bi.bmiHeader.biHeight = -needH;              // 自顶向下
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                void* bits = nullptr;
                m_ctxBmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
                if (m_ctxBmp == nullptr) { m_ctxDirty = true; m_ctxDc = nullptr; }
                else
                {
                    m_ctxDc = ::CreateCompatibleDC(nullptr);
                    ::SelectObject(m_ctxDc, m_ctxBmp);
                    m_ctxW = rc.right;
                    m_ctxH = needH;
                }
            }
            if (m_ctxDc != nullptr)
            {
                RECT all = { 0, 0, rc.right, needH };
                ::FillRect(m_ctxDc, &all, ::GetSysColorBrush(COLOR_WINDOW));
                if (m_font != nullptr) ::SelectObject(m_ctxDc, m_font);
                ::SetBkMode(m_ctxDc, TRANSPARENT);
                for (int k = 0; k < nRows; ++k)
                    paintRow(m_ctxDc, m_view[from + k], k * m_rowH);
                m_ctxFrom = from;
                m_ctxRows = nRows;
                m_ctxX = m_colScroll;
                m_ctxDirty = false;
            }
        }

        if (!m_ctxDirty && m_ctxDc != nullptr)
        {
            const int srcY = (m_scroll - m_ctxFrom) * m_rowH;
            const int copyH = std::min(contentH, m_ctxRows * m_rowH - srcY);
            if (copyH > 0 && srcY >= 0)
            {
                ::BitBlt(dc, 0, contentBand.top, rc.right, copyH, m_ctxDc, 0, srcY, SRCCOPY);
            }
            if (copyH < contentH)
            {
                RECT rest = { rc.left, contentBand.top + std::max(0, copyH), rc.right, contentBand.bottom };
                if (!isEmpty(rest)) ::FillRect(dc, &rest, ::GetSysColorBrush(COLOR_WINDOW));
            }
        }
    }
    else
    {
        for (int pos = 0; pos < rows; ++pos)
        {
            const int yTop = contentBand.top + pos * m_rowH - m_scroll * m_rowH;
            if (yTop >= contentBand.bottom) break;
            if (yTop + m_rowH <= contentBand.top) continue;
            paintRow(dc, m_view[pos], yTop);
        }
    }
    // 右侧自绘细滚动条（仅行数超出可视区时出现拇指）
    const RECT track = SbTrack();
    ::FillRect(dc, &track, brushOf(m_brTrack, m_brTrackC, RGB(0xF0, 0xF2, 0xF5)));
    if (MaxScroll() > 0)
    {
        int thumbY = 0, thumbH = 0;
        SbThumb(thumbY, thumbH);
        RECT thumb = { track.left, thumbY, track.right, thumbY + thumbH };
        ::FillRect(dc, &thumb, brushOf(m_brThumb, m_brThumbC, RGB(0xC2, 0xCA, 0xD4)));
    }

    // 底部自绘横向滚动条：列宽合计超过可视宽度时才出现（列多时右侧列可达）
    if (HasHBar())
    {
        const RECT trk = SbTrackX();
        ::FillRect(dc, &trk, brushOf(m_brTrack, m_brTrackC, RGB(0xF0, 0xF2, 0xF5)));
        int thL = 0, thW = 0;
        SbThumbX(thL, thW);
        RECT th = { thL, trk.top, thL + thW, trk.bottom };
        ::FillRect(dc, &th, brushOf(m_brThumb, m_brThumbC, RGB(0xC2, 0xCA, 0xD4)));
    }

    if (oldFont != nullptr)
        ::SelectObject(dc, oldFont);

    ::BitBlt(dcScreen, 0, 0, rc.right, rc.bottom, m_memDc, 0, 0, SRCCOPY);
    if (m_fp != nullptr) DrawFilterPanel(m_fp, dcScreen);   // 表头漏斗面板叠在表格上
    ::EndPaint(m_hwnd, &ps);
    return 0;
}
