import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = "\n\nLRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)"
new = "\n\nstatic LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)"
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('补回 PyMfcOwnerDrawProc 的 static')
