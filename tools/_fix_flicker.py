import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# 1) moveMany：子控件不再逐个擦背景（擦白就是"闪一下"的直接来源）
old = """        for (HWND c : moved)
            ::InvalidateRect(c, nullptr, TRUE);"""
new = """        // erase=FALSE：控件都是整块自绘（画布自己 fillRect 底色、原生控件自己画底），
        // 带 TRUE 会先擦成一窗白再画，肉眼就是"闪一下"。
        for (HWND c : moved)
            ::InvalidateRect(c, nullptr, FALSE);"""
assert t.count(old) == 1
t = t.replace(old, new)

# 2) 缩放/平移画布：不擦背景（drawPlanCanvas 首行已 fillRect 整块底）
old2 = """        st.ox = offsetX;
        st.oy = offsetY;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, TRUE);"""
new2 = """        st.ox = offsetX;
        st.oy = offsetY;
        ::InvalidateRect(::GetDlgItem(m_hWnd, id), nullptr, FALSE);"""
assert t.count(old2) == 1
t = t.replace(old2, new2)

# 3) 主窗加 WS_EX_COMPOSITED：整窗离屏合成，根治重排/滚动时的整屏闪
old3 = """        HWND hwnd = ::CreateWindowExW(
            WS_EX_APPWINDOW, L"PyMfcNativeWindow", title.c_str(),"""
new3 = """        HWND hwnd = ::CreateWindowExW(
            // WS_EX_COMPOSITED：整窗底部先画到离屏位图再一次性提交，
            // 布局重排/多控件同时刷新时不再看到"一块块补上"的闪烁。
            WS_EX_APPWINDOW | WS_EX_COMPOSITED, L"PyMfcNativeWindow", title.c_str(),"""
assert t.count(old3) == 1
t = t.replace(old3, new3)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('闪烁三处已改')
