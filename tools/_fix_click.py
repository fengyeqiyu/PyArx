import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """    case WM_LBUTTONUP:
    {
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        const bool wasDrag = st.drag;
        st.drag = false;
        if (wasDrag && !st.moved)"""
new = """    case WM_LBUTTONUP:
    {
        // 顺序很关键：ReleaseCapture 会同步发出 WM_CAPTURECHANGED，
        // 那个分支会把 st.drag 清成 false —— 先 Release 再读，这一击就被当成
        // "从没按下过"，卡片永远选不中。所以先把状态取下来再释放捕获。
        const bool wasDrag = st.drag;
        st.drag = false;
        if (::GetCapture() == hwnd)
            ::ReleaseCapture();
        if (wasDrag && !st.moved)"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('单击丢失根因已修')
