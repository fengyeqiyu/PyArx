import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# ---- 1) 成员与 fireGridAction：放在 m_change 附近 ----
old = "    std::map<UINT, py::object> m_change;"
new = """    std::map<UINT, py::object> m_change;

    // ---- 可编辑表格（SysListView32）的键盘/排序/复制粘贴支持 ----
    // 当前活动单元格：控件 id → (行, 列)。ListView 本身没有"活动单元格"概念，
    // 只能在 WM_LBUTTONDOWN 时用 LVM_SUBITEMHITTEST 记下来给 F2/Delete 用。
    std::map<int, std::pair<int, int> > m_gridCell;
    py::object m_gridAction;

    void onGridAction(py::object cb) { m_gridAction = std::move(cb); }

    void fireGridAction(const char* action, int row, int col, const std::wstring& text)
    {
        if (!m_gridAction)
            return;
        py::gil_scoped_acquire gil;
        try
        {
            m_gridAction(std::string(action), row, col, text);
        }
        catch (const py::error_already_set& e)
        {
            debugLogf(L"[grid] 回调异常: %hs", e.what());
        }
    }"""
assert t.count(old) == 1, 'm_change'
t = t.replace(old, new)

# ---- 2) 剪贴板 + 子类化过程：插在 PyMfcPlanCanvasMouse 定义之前 ----
anchor = ("static LRESULT PyMfcPlanCanvasMouse(PyMfcHost* host, HWND hwnd, UINT msg,\n"
          "                                    WPARAM wParam, LPARAM lParam, bool& handled)\n{")
GRID = """// ---------------------------------------------------------------------------
// 可编辑表格：SysListView32 本身不能编辑单元格，也没有 Ctrl+C/V / F2 / Insert 语义。
// 这里给它子类化一层窗口过程：记录活动单元格、拦键盘，把动作回抛给 Python
// （Python 手里才是真正的数据源，由它改完再 setTableCell 写回来，两边不会打架）。
static constexpr const wchar_t* kGridHostProp = L"PyMfcGridHost";
static constexpr const wchar_t* kGridOldProp = L"PyMfcGridOldProc";

static std::wstring gridCollectText(HWND lv)
{
    std::wstring out;
    int item = -1;
    bool first = true;
    while (true)
    {
        item = static_cast<int>(::SendMessageW(lv, LVM_GETNEXTITEM,
                             static_cast<WPARAM>(item), LVNI_SELECTED));
        if (item < 0)
            break;
        if (const HWND hdr = reinterpret_cast<HWND>(::SendMessageW(lv, LVM_GETHEADER, 0, 0)))
        {
            const int cols = static_cast<int>(::SendMessageW(hdr, HDM_GETITEMCOUNT, 0, 0));
            if (!first)
                out += L"\\r\\n";
            first = false;
            for (int c = 0; c < cols; ++c)
            {
                wchar_t buf[512] = {};
                LVITEMW it = {};
                it.iSubItem = c;
                it.pszText = buf;
                it.cchTextMax = 512;
                ::SendMessageW(lv, LVM_GETITEMTEXTW, static_cast<WPARAM>(item),
                               reinterpret_cast<LPARAM>(&it));
                if (c > 0)
                    out += L"\\t";
                out += buf;
            }
        }
    }
    return out;
}

static void gridSetClipboard(HWND owner, const std::wstring& text)
{
    if (!::OpenClipboard(owner))
        return;
    ::EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL mem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes))
    {
        if (void* p = ::GlobalLock(mem))
        {
            memcpy(p, text.c_str(), bytes);
            ::GlobalUnlock(mem);
            ::SetClipboardData(CF_UNICODETEXT, mem);   // 成功后内存归系统所有，不能再释放
        }
    }
    ::CloseClipboard();
}

static std::wstring gridGetClipboard(HWND owner)
{
    std::wstring out;
    if (!::OpenClipboard(owner))
        return out;
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

static LRESULT CALLBACK PyMfcGridProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, kGridHostProp));
    if (host != nullptr)
    {
        const int id = ::GetDlgCtrlID(hwnd);
        if (msg == WM_LBUTTONDOWN)
        {
            LVHITTESTINFO hi = {};
            hi.pt.x = GET_X_LPARAM(lParam);
            hi.pt.y = GET_Y_LPARAM(lParam);
            ::SendMessageW(hwnd, LVM_SUBITEMHITTEST, 0, reinterpret_cast<LPARAM>(&hi));
            host->m_gridCell[id] = std::make_pair(hi.iItem, hi.iSubItem);
        }
        else if (msg == WM_KEYDOWN)
        {
            auto it = host->m_gridCell.find(id);
            const int row = it != host->m_gridCell.end() ? it->second.first : -1;
            const int col = it != host->m_gridCell.end() ? it->second.second : 0;
            const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (wParam == VK_F2)
            {
                host->fireGridAction("edit", row, col, L"");
                return 0;
            }
            if (wParam == VK_DELETE)
            {
                host->fireGridAction("delete", row, col, L"");
                return 0;
            }
            if (wParam == VK_INSERT)
            {
                host->fireGridAction("insert", row, col, L"");
                return 0;
            }
            if (ctrl && (wParam == 'C' || wParam == 'c'))
            {
                const std::wstring s = gridCollectText(hwnd);
                if (!s.empty())
                    gridSetClipboard(hwnd, s);
                return 0;
            }
            if (ctrl && (wParam == 'V' || wParam == 'v'))
            {
                host->fireGridAction("paste", row, col, gridGetClipboard(hwnd));
                return 0;
            }
        }
    }
    if (WNDPROC old = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, kGridOldProp)))
        return ::CallWindowProcW(old, hwnd, msg, wParam, lParam);
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

"""
assert t.count(anchor) == 1
t = t.replace(anchor, GRID + anchor, 1)

# ---- 3) addListCtrl 里挂钩子 ----
old3 = """        ::SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,"""
idx = t.index(old3)
seg_end = t.index(';', idx) + 1
insert = """
            // 可编辑表格钩子：记录活动单元格 + 拦 F2/Delete/Insert/Ctrl+C/V
            ::SetPropW(hwnd, kGridHostProp, reinterpret_cast<HANDLE>(this));
            {
                WNDPROC gridOld = reinterpret_cast<WNDPROC>(
                    ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                        reinterpret_cast<LONG_PTR>(&PyMfcGridProc)));
                ::SetPropW(hwnd, kGridOldProp, reinterpret_cast<HANDLE>(gridOld));
            }"""
t = t[:seg_end] + insert + t[seg_end:]

# ---- 4) OnNotify 加列头排序 ----
old4 = """        case NM_CLICK:          // SysLink（超链接）：点它只发 NM_CLICK，没有 BN_CLICKED
        case NM_RETURN:"""
new4 = """        case LVN_COLUMNCLICK:    // 点列头排序（数据顺序由 Python 决定，这里只报列号）
        {
            NMLISTVIEW* nv = reinterpret_cast<NMLISTVIEW*>(lParam);
            fireGridAction("sort", -1, nv != nullptr ? nv->iSubItem : 0, L"");
            break;
        }
        case NM_CLICK:          // SysLink（超链接）：点它只发 NM_CLICK，没有 BN_CLICKED
        case NM_RETURN:"""
assert t.count(old4) == 1
t = t.replace(old4, new4)

# ---- 5) wrapper + def ----
old5 = "    void addTableRow(int id, const std::wstring& s) { m_host.addTableRow(id, s); }"
new5 = """    void addTableRow(int id, const std::wstring& s) { m_host.addTableRow(id, s); }
    void onGridAction(py::object cb) { m_host.onGridAction(std::move(cb)); }"""
assert t.count(old5) == 1
t = t.replace(old5, new5)

old6 = "    void addTableRow(int id, const std::wstring& s) { m_host->addTableRow(id, s); }"
new6 = """    void addTableRow(int id, const std::wstring& s) { m_host->addTableRow(id, s); }
    void onGridAction(py::object cb) { m_host->onGridAction(std::move(cb)); }"""
assert t.count(old6) == 1
t = t.replace(old6, new6)

old7 = '        .def("addTableRow", &PyMfcWindow::addTableRow)\n        .def("clearTableRows", &PyMfcWindow::clearTableRows)'
new7 = ('        .def("addTableRow", &PyMfcWindow::addTableRow)\n'
        '        .def("onGridAction", &PyMfcWindow::onGridAction,\n'
        '             "表格按键/排序回调 cb(action, row, col, text)")\n'
        '        .def("clearTableRows", &PyMfcWindow::clearTableRows)')
assert t.count(old7) == 1
t = t.replace(old7, new7)

old8 = '        .def("addTableRow", &PyMfcPanel::addTableRow)\n        .def("clearTableRows", &PyMfcPanel::clearTableRows)'
new8 = ('        .def("addTableRow", &PyMfcPanel::addTableRow)\n'
        '        .def("onGridAction", &PyMfcPanel::onGridAction,\n'
        '             "表格按键/排序回调 cb(action, row, col, text)")\n'
        '        .def("clearTableRows", &PyMfcPanel::clearTableRows)')
assert t.count(old8) == 1
t = t.replace(old8, new8)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('表格键盘/排序/复制粘贴 已加入')
