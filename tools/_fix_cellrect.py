import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

old = """    // 单元格矩形（ListView 客户区坐标）：原地编辑要把 EDIT 精确盖到那一格上
    py::object cellRect(int id, int row, int col)
    {
        HWND h = ::GetDlgItem(m_hWnd, id);
        if (h == nullptr || !::IsWindow(h))
            return py::none();
        RECT r = { 0, 0, 0, 0 };
        r.left = col;                       // LVM_GETSUBITEMRECT 用 left 成员传子项号
        ::SendMessageW(h, LVM_GETSUBITEMRECT, static_cast<WPARAM>(row),
                       reinterpret_cast<LPARAM>(&r));
        return py::make_tuple(r.left, r.top, r.right - r.left, r.bottom - r.top);
    }"""
new = """    // 递归按 id 找子窗口：表格可能挂在侧板里，不是主窗的直接子窗口，
    // 只 GetDlgItem(m_hWnd, id) 会拿不到句柄（拿到别的控件时 LVM_* 全部无效）
    static HWND findByIdDeep(HWND parent, int id)
    {
        HWND direct = ::GetDlgItem(parent, id);
        if (direct != nullptr && ::IsWindow(direct))
            return direct;
        for (HWND c = ::GetWindow(parent, GW_CHILD); c != nullptr;
             c = ::GetWindow(c, GW_HWNDNEXT))
        {
            if (HWND hit = findByIdDeep(c, id))
                return hit;
        }
        return nullptr;
    }

    // 单元格矩形（ListView 客户区坐标）：原地编辑要把 EDIT 精确盖到那一格上
    py::object cellRect(int id, int row, int col)
    {
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return py::none();
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") != 0)
            return py::none();
        RECT r = { 0, 0, 0, 0 };
        r.left = col;                       // LVM_GETSUBITEMRECT 用 left 成员传子项号
        const LRESULT ok = ::SendMessageW(h, LVM_GETSUBITEMRECT,
                                          static_cast<WPARAM>(row),
                                          reinterpret_cast<LPARAM>(&r));
        debugLogf(L"[cellrect] id=%d row=%d col=%d ok=%d rect=(%d,%d,%d,%d)",
                  id, row, col, static_cast<int>(ok), r.left, r.top, r.right, r.bottom);
        if (r.right <= r.left || r.bottom <= r.top)
            return py::none();
        return py::make_tuple(r.left, r.top, r.right - r.left, r.bottom - r.top);
    }"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('cellRect 改为递归找表 + 校验')
