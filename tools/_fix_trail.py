import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """        for (HWND c : moved)
            ::InvalidateRect(c, nullptr, FALSE);"""
new = """        for (HWND c : moved)
        {
            ::InvalidateRect(c, nullptr, FALSE);
            // 关键：控件挪走后，旧位置是**父容器**的地盘。父容器自己不被标脏，
            // 主窗又被 WS_CLIPCHILDREN 裁掉这一块 —— 旧内容就留在屏幕上，即"拖影"。
            if (HWND p = ::GetParent(c); p != nullptr && p != m_hWnd)
                ::InvalidateRect(p, nullptr, FALSE);
        }"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('父容器一起重画')
