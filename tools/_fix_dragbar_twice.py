import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """        WNDPROC old = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcDragBarProc)));
        if (old == nullptr)
            return false;"""
new = """        WNDPROC old = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PyMfcDragBarProc)));
        if (old == nullptr)
            return false;
        if (old == &PyMfcDragBarProc)                   // 重复安装：否则会自己转发给自己（栈溢出）
            return true;"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('installDragBar 防重复安装')
