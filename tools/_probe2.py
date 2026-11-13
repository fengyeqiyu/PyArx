import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
# 找到目标 proc 的 WM_PAINT：带 DIS/ODT_STATIC 那段
marker = """    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps = {};
        HDC hdc = ::BeginPaint(hwnd, &ps);
        HWND parent = ::GetParent(hwnd);"""
print('marker count:', t.count(marker))
for m in ('DIS'): pass
