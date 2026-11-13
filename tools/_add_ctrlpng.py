import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

anchor = """    // 按编号选中并把该卡片移到视口中央（搜索定位用；也方便外部脚本/测试驱动）。"""
FN = """    // ---- 控件级导出（基类能力，任意控件都能调）----
    // PrintWindow 让控件按正常绘制路径画进内存 DIB，再交给 WIC 写 PNG。
    // 自绘控件（含 D2D 画布）在 PyMfcOwnerDrawProc 的 WM_PRINTCLIENT 分支里重画，
    // 抓到的就是屏幕上看到的样子 —— 验收/回归不必再抓真屏幕。
    bool saveControlPng(int id, const std::wstring& path)
    {
        HWND c = ::GetDlgItem(m_hWnd, id);
        if (c == nullptr || !::IsWindow(c))
            return false;
        RECT wr = {};
        if (!::GetWindowRect(c, &wr))
            return false;
        const int w = wr.right - wr.left;
        const int hh = wr.bottom - wr.top;
        if (w <= 0 || hh <= 0)
            return false;

        HDC screen = ::GetDC(nullptr);
        HDC mem = ::CreateCompatibleDC(screen);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -hh;          // 自顶向下，与 wicWritePng 行序一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bmp = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ oldBmp = (bmp != nullptr) ? ::SelectObject(mem, bmp) : nullptr;

        RECT full = { 0, 0, w, hh };
        HBRUSH bg = ::CreateSolidBrush(RGB(0xF3, 0xF5, 0xF8));
        ::FillRect(mem, &full, bg);
        ::DeleteObject(bg);

        // PW_RENDERFULLCONTENT(2)：内容渲染型窗口也能抓
        ::PrintWindow(c, mem, 2);
        ::GdiFlush();

        bool wrote = false;
        if (bits != nullptr)
            wrote = wicWritePng(path, w, hh, bits);

        if (oldBmp != nullptr)
            ::SelectObject(mem, oldBmp);
        if (bmp != nullptr)
            ::DeleteObject(bmp);
        ::DeleteDC(mem);
        ::ReleaseDC(nullptr, screen);
        return wrote;
    }

""" + anchor
assert t.count(anchor) == 1
t = t.replace(anchor, FN)

a1 = "    void setCurSel(int id, int index) { m_host.setCurSel(id, index); }"
a2 = "    void setCurSel(int id, int index) { m_host->setCurSel(id, index); }"
assert t.count(a1) == 1 and t.count(a2) == 1
t = t.replace(a1, a1 + "\n    bool saveControlPng(int id, const std::wstring& path) { return m_host.saveControlPng(id, path); }")
t = t.replace(a2, a2 + "\n    bool saveControlPng(int id, const std::wstring& path) { return m_host->saveControlPng(id, path); }")

old4 = '.def("getPlanSelection", &PyMfcWindow::getPlanSelection,'
idx = t.find(old4)
assert idx > 0
line_end = t.find('\n', idx)
t = (t[:line_end + 1] +
     '        .def("saveControlPng", &PyMfcWindow::saveControlPng,\n'
     '             "把控件按当前外观导出成 PNG（截图取证）")\n' + t[line_end + 1:])
old5 = '.def("getPlanSelection", &PyMfcPanel::getPlanSelection)'
assert t.count(old5) == 1
t = t.replace(old5, old5 + '\n        .def("saveControlPng", &PyMfcPanel::saveControlPng,\n'
                           '             "把控件按当前外观导出成 PNG（截图取证）")')
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('saveControlPng 已加')
