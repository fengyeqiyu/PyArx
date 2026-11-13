import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

old = """        RECT r = { 0, 0, 0, 0 };
        r.left = col;                       // LVM_GETSUBITEMRECT 用 left 成员传子项号
        const LRESULT ok = ::SendMessageW(h, LVM_GETSUBITEMRECT,
                                          static_cast<WPARAM>(row),
                                          reinterpret_cast<LPARAM>(&r));
        debugLogf(L"[cellrect] id=%d row=%d col=%d ok=%d rect=(%d,%d,%d,%d)",
                  id, row, col, static_cast<int>(ok), r.left, r.top, r.right, r.bottom);
        if (r.right <= r.left || r.bottom <= r.top)
            return py::none();
        return py::make_tuple(r.left, r.top, r.right - r.left, r.bottom - r.top);"""
new = """        // LVM_GETSUBITEMRECT 在本机 SysListView32 上水平方向恒返回 0 宽（left==right），
        // 拿不到单元格左右边界。改成"行矩形 + 累加列宽"自己算，实测稳定。
        RECT rowRc = { 0, 0, 0, 0 };
        if (::SendMessageW(h, LVM_GETITEMRECT, static_cast<WPARAM>(row),
                           reinterpret_cast<LPARAM>(&rowRc)) == FALSE)
            return py::none();
        int colX = 0, colW = 0;
        for (int i = 0; i <= col; ++i)
        {
            const int cw = static_cast<int>(::SendMessageW(h, LVM_GETCOLUMNWIDTH,
                                                           static_cast<WPARAM>(i), 0));
            if (cw <= 0)
                return py::none();
            if (i < col)
                colX += cw;
            else
                colW = cw;
        }
        POINT org = { 0, 0 };
        ::SendMessageW(h, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
        const int x = colX - org.x;         // 横向滚动补偿（表格无横滚时 org.x=0）
        const int hgt = rowRc.bottom - rowRc.top;
        debugLogf(L"[cellrect] id=%d row=%d col=%d -> (%d,%d,%d,%d)",
                  id, row, col, x, rowRc.top, colW, hgt);
        if (colW <= 0 || hgt <= 0)
            return py::none();
        return py::make_tuple(x, rowRc.top, colW, hgt);"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('cellRect 改用 行矩形+列宽')
